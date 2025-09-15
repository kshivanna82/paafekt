// MidasDepth.cpp

// Class header MUST be first for Unreal
#include "MidasDepth.h"

// Then undefine Apple macros before OpenCV
#ifdef NO
    #undef NO
#endif
#ifdef YES
    #undef YES
#endif
#ifdef check
    #undef check
#endif

// Now include OpenCV
#include <opencv2/opencv.hpp>

// Then other Unreal headers
#include "ImagePlateComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

// Apple/iOS headers LAST
#if PLATFORM_IOS
#include <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "CoreMLModelBridge.h"
#endif

// ... rest of the implementation stays the same ...
//
//// Include OpenCV FIRST
//#include "OpenCVIncludes.h"
//
//// Then include the class header and Unreal headers
//#include "MidasDepth.h"
//#include "ImagePlateComponent.h"
//#include "Engine/Texture2D.h"
//#include "Materials/MaterialInstanceDynamic.h"
//#include "GameFramework/PlayerStart.h"
//#include "Engine/World.h"
//#include "Kismet/GameplayStatics.h"
//
//// Apple/iOS headers LAST
//#if PLATFORM_IOS
//#include <Foundation/Foundation.h>
//#import <AVFoundation/AVFoundation.h>
//#import "CoreMLModelBridge.h"
//#endif

AMidasDepth* AMidasDepth::CurrentInstance = nullptr;

AMidasDepth::AMidasDepth(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    
    rootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(rootComp);
    
    // Initialize OpenCV objects as pointers
    cap = new cv::VideoCapture();
    frame = new cv::Mat();
    resized = new cv::Mat();
    depthResult = new cv::Mat();
    cvMat = new cv::Mat();
    cvSize = new cv::Size();
    
    ImagePlateDepth = nullptr;
    CameraID = 0;
    isStreamOpen = false;
    VideoSize = FVector2D(640, 480);
}

AMidasDepth::~AMidasDepth()
{
    // Clean up OpenCV objects
    if (cap) {
        if (cap->isOpened())
            cap->release();
        delete cap;
    }
    delete frame;
    delete resized;
    delete depthResult;
    delete cvMat;
    delete cvSize;
}

void AMidasDepth::BeginPlay()
{
    Super::BeginPlay();
    
    UE_LOG(LogTemp, Warning, TEXT("MidasDepth starting"));
    AMidasDepth::CurrentInstance = this;
    
    // Set input mode
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC)
    {
        FInputModeGameOnly GameMode;
        PC->SetInputMode(GameMode);
        PC->bShowMouseCursor = false;
        
#if PLATFORM_IOS
        PC->bEnableClickEvents = false;
        PC->bEnableTouchEvents = true;
        PC->bEnableTouchOverEvents = true;
#endif
    }
    
#if PLATFORM_IOS
    AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    
    if (status == AVAuthorizationStatusNotDetermined) {
        [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
            if (granted) {
                AsyncTask(ENamedThreads::GameThread, [this]() {
                    InitializeCamera();
                });
            }
        }];
    } else if (status == AVAuthorizationStatusAuthorized) {
        InitializeCamera();
    }
#else
    InitializeCamera();
#endif
}

void AMidasDepth::InitializeCamera()
{
    UE_LOG(LogTemp, Warning, TEXT("MidasDepth: Initializing camera"));
    
    // Open camera
    cap->open(CameraID);
    if (!cap->isOpened()) {
#if PLATFORM_IOS
        for (int i = 0; i < 3; i++) {
            cap->open(i);
            if (cap->isOpened()) {
                CameraID = i;
                break;
            }
        }
#endif
        if (!cap->isOpened()) {
            UE_LOG(LogTemp, Error, TEXT("MidasDepth: Failed to open camera"));
            return;
        }
    }
    
    cap->set(cv::CAP_PROP_BUFFERSIZE, 1);
    cap->set(cv::CAP_PROP_FPS, 30);
    
#if PLATFORM_IOS
    VideoSize = FVector2D(480, 360);
#else
    cv::Mat testFrame;
    if (cap->read(testFrame) && !testFrame.empty()) {
        VideoSize = FVector2D(testFrame.cols, testFrame.rows);
    }
#endif
    
    isStreamOpen = true;
    
    // Initialize buffers
    ColorData.SetNumUninitialized(VideoSize.X * VideoSize.Y);
    *cvSize = cv::Size(VideoSize.X, VideoSize.Y);
    *cvMat = cv::Mat(*cvSize, CV_8UC4, ColorData.GetData());
    
    // Create textures
    Camera_Texture2D = UTexture2D::CreateTransient(VideoSize.X, VideoSize.Y, PF_B8G8R8A8);
    Depth_Texture2D = UTexture2D::CreateTransient(VideoSize.X, VideoSize.Y, PF_B8G8R8A8);
    
    if (!Camera_Texture2D || !Depth_Texture2D) {
        UE_LOG(LogTemp, Error, TEXT("MidasDepth: Failed to create textures"));
        isStreamOpen = false;
        return;
    }
    
#if WITH_EDITORONLY_DATA
    Camera_Texture2D->MipGenSettings = TMGS_NoMipmaps;
    Depth_Texture2D->MipGenSettings = TMGS_NoMipmaps;
#endif
    
    Camera_Texture2D->SRGB = true;
    Depth_Texture2D->SRGB = true;
    Camera_Texture2D->AddToRoot();
    Depth_Texture2D->AddToRoot();
    Camera_Texture2D->UpdateResource();
    Depth_Texture2D->UpdateResource();
    
    // Create ImagePlate dynamically
    if (!ImagePlateDepth)
    {
        ImagePlateDepth = NewObject<UImagePlateComponent>(this, TEXT("DepthImagePlate"));
        ImagePlateDepth->SetupAttachment(RootComponent);
        ImagePlateDepth->RegisterComponent();
        
        ImagePlateDepth->bAutoActivate = true;
        ImagePlateDepth->SetVisibility(true);
        ImagePlateDepth->SetHiddenInGame(false);
    }
    
    // Position ImagePlate using UGameplayStatics instead of TActorIterator
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundActors);
    
    if (FoundActors.Num() > 0)
    {
        APlayerStart* PlayerStart = Cast<APlayerStart>(FoundActors[0]);
        if (PlayerStart && ImagePlateDepth)
        {
            FVector ForwardOffset = PlayerStart->GetActorForwardVector() * 300.0f;
            FVector PlacementLocation = PlayerStart->GetActorLocation() + ForwardOffset + FVector(0, 0, 100);
            
            ImagePlateDepth->SetWorldLocation(PlacementLocation);
            ImagePlateDepth->SetWorldRotation(PlayerStart->GetActorRotation());
            ImagePlateDepth->SetRelativeScale3D(FVector(4.0f, 3.0f, 1.0f));
            
            // Create or load material
            UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Game/FurniLife/DepthMaterial.DepthMaterial"));
            
            if (!BaseMaterial)
            {
                // Create simple material if not found
                Depth_Material = NewObject<UMaterialInstanceDynamic>(this);
            }
            else
            {
                Depth_Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
            }
            
            if (Depth_Material)
            {
                ImagePlateDepth->SetMaterial(0, Depth_Material);
                Depth_Material->SetTextureParameterValue(FName("DepthTexture"), Depth_Texture2D);
                Depth_Material->SetTextureParameterValue(FName("CameraTexture"), Camera_Texture2D);
            }
        }
    }
    
#if PLATFORM_IOS
    // Load MiDaS CoreML model
    NSString* ModelPath = [[NSBundle mainBundle] pathForResource:@"midas_v2" ofType:@"mlmodelc"];
    if (!ModelPath) {
        // Try alternative name
        ModelPath = [[NSBundle mainBundle] pathForResource:@"MiDaS" ofType:@"mlmodelc"];
    }
    
    if (ModelPath && GetSharedBridge()) {
        if (GetSharedBridge()->LoadModel([ModelPath UTF8String])) {
            UE_LOG(LogTemp, Warning, TEXT("✅ MiDaS model loaded"));
        } else {
            UE_LOG(LogTemp, Error, TEXT("❌ Failed to load MiDaS model"));
        }
    } else {
        UE_LOG(LogTemp, Error, TEXT("❌ MiDaS model not found in bundle"));
    }
#endif
    
    UE_LOG(LogTemp, Warning, TEXT("✅ MidasDepth initialization complete"));
}

void AMidasDepth::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (!isStreamOpen)
        return;
    
    static float TimeSinceLastFrame = 0;
    TimeSinceLastFrame += DeltaTime;
    
    // Process at 10 FPS for depth (more computationally intensive)
    if (TimeSinceLastFrame >= 1.0f / 10.0f)
    {
        TimeSinceLastFrame = 0;
        ReadFrame();
    }
}

bool AMidasDepth::ReadFrame()
{
    if (!cap->isOpened())
        return false;
    
    if (!cap->read(*frame) || frame->empty())
        return false;
    
#if PLATFORM_IOS
    cv::rotate(*frame, *frame, cv::ROTATE_90_CLOCKWISE);
    
    static bool bFirstFrame = true;
    if (bFirstFrame && !frame->empty())
    {
        VideoSize = FVector2D(frame->cols, frame->rows);
        
        if (Camera_Texture2D && (Camera_Texture2D->GetSizeX() != VideoSize.X ||
            Camera_Texture2D->GetSizeY() != VideoSize.Y))
        {
            Camera_Texture2D->RemoveFromRoot();
            Depth_Texture2D->RemoveFromRoot();
            
            Camera_Texture2D = UTexture2D::CreateTransient(VideoSize.X, VideoSize.Y, PF_B8G8R8A8);
            Depth_Texture2D = UTexture2D::CreateTransient(VideoSize.X, VideoSize.Y, PF_B8G8R8A8);
            
            Camera_Texture2D->SRGB = true;
            Depth_Texture2D->SRGB = true;
            Camera_Texture2D->AddToRoot();
            Depth_Texture2D->AddToRoot();
            
            if (Depth_Material)
            {
                Depth_Material->SetTextureParameterValue(FName("DepthTexture"), Depth_Texture2D);
                Depth_Material->SetTextureParameterValue(FName("CameraTexture"), Camera_Texture2D);
            }
        }
        bFirstFrame = false;
    }
#endif
    
    // Process depth estimation
    ProcessDepthEstimation();
    
    // Update camera texture
    if (frame->channels() == 3)
        cv::cvtColor(*frame, *frame, cv::COLOR_BGR2BGRA);
    
    if (frame->cols != VideoSize.X || frame->rows != VideoSize.Y)
        cv::resize(*frame, *frame, cv::Size(VideoSize.X, VideoSize.Y));
    
    // Update camera texture
    if (Camera_Texture2D && Camera_Texture2D->GetPlatformData())
    {
        FTexture2DMipMap& Mip = Camera_Texture2D->GetPlatformData()->Mips[0];
        void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
        
        if (Data)
        {
            FMemory::Memcpy(Data, frame->data, frame->total() * frame->elemSize());
            Mip.BulkData.Unlock();
            Camera_Texture2D->UpdateResource();
        }
    }
    
    return true;
}

void AMidasDepth::ProcessDepthEstimation()
{
    // MiDaS typically uses 384x384 or 256x256 input
    const int ModelInputSize = 384;
    
    cv::resize(*frame, *resized, cv::Size(ModelInputSize, ModelInputSize));
    cv::cvtColor(*resized, *resized, cv::COLOR_BGR2RGB);
    resized->convertTo(*resized, CV_32FC3, 1.0 / 255.0);
    
    // Normalize for MiDaS (ImageNet normalization)
    cv::Scalar mean(0.485, 0.456, 0.406);
    cv::Scalar std(0.229, 0.224, 0.225);
    
    cv::Mat channels[3];
    cv::split(*resized, channels);
    
    for (int i = 0; i < 3; i++)
    {
        channels[i] = (channels[i] - mean[i]) / std[i];
    }
    
    cv::merge(channels, 3, *resized);
    
    RunMidasInference();
    ApplyDepthVisualization();
}

void AMidasDepth::RunMidasInference()
{
#if PLATFORM_IOS
    const int Width = 384;
    const int Height = 384;
    
    // Convert to RGBA for CVPixelBuffer
    cv::Mat rgba;
    if (resized->channels() == 3) {
        cv::cvtColor(*resized, rgba, cv::COLOR_RGB2RGBA);
    } else {
        rgba = *resized;
    }
    
    // Convert back to 8-bit for CVPixelBuffer
    cv::Mat rgba8;
    rgba.convertTo(rgba8, CV_8UC4, 255.0);
    
    CVPixelBufferRef pixelBuffer = nullptr;
    
    // Create attributes dictionary using explicit BOOL values
    NSDictionary* attrs = [NSDictionary dictionaryWithObjectsAndKeys:
        [NSNumber numberWithBool:true], (NSString*)kCVPixelBufferCGImageCompatibilityKey,
        [NSNumber numberWithBool:true], (NSString*)kCVPixelBufferCGBitmapContextCompatibilityKey,
        nil];
    
    CVReturn status = CVPixelBufferCreate(kCFAllocatorDefault,
                                          Width, Height,
                                          kCVPixelFormatType_32BGRA,
                                          (__bridge CFDictionaryRef)attrs,
                                          &pixelBuffer);
    
    if (status != kCVReturnSuccess || !pixelBuffer) {
        return;
    }
    
    CVPixelBufferLockBaseAddress(pixelBuffer, 0);
    uint8_t* dstBase = (uint8_t*)CVPixelBufferGetBaseAddress(pixelBuffer);
    size_t dstBytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);
    
    for (int y = 0; y < Height; ++y)
    {
        const cv::Vec4b* srcRow = rgba8.ptr<cv::Vec4b>(y);
        uint8_t* dstRow = dstBase + y * dstBytesPerRow;
        for (int x = 0; x < Width; ++x)
        {
            const cv::Vec4b& px = srcRow[x];
            dstRow[x * 4 + 0] = px[2]; // B
            dstRow[x * 4 + 1] = px[1]; // G
            dstRow[x * 4 + 2] = px[0]; // R
            dstRow[x * 4 + 3] = px[3]; // A
        }
    }
    
    CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
    
    std::vector<float> OutputData;
    if (GetSharedBridge() && GetSharedBridge()->RunModel(pixelBuffer, OutputData)) {
        DepthOutputBuffer = std::move(OutputData);
    }
    
    CVPixelBufferRelease(pixelBuffer);
#endif
}

void AMidasDepth::ApplyDepthVisualization()
{
    if (DepthOutputBuffer.empty())
        return;
    
    const int Width = 384;
    const int Height = 384;
    
    // Create depth map from output
    cv::Mat depthMap(Height, Width, CV_32FC1, DepthOutputBuffer.data());
    
    // Normalize depth values
    double minVal, maxVal;
    cv::minMaxLoc(depthMap, &minVal, &maxVal);
    
    cv::Mat normalizedDepth;
    depthMap.convertTo(normalizedDepth, CV_8UC1, 255.0 / (maxVal - minVal), -minVal * 255.0 / (maxVal - minVal));
    
    // Create visualization based on mode
    cv::Mat visualization;
    
    switch (static_cast<DepthVisMode>(VisualizationMode))
    {
        case DepthVisMode::Grayscale:
            cv::cvtColor(normalizedDepth, visualization, cv::COLOR_GRAY2BGRA);
            break;
            
        case DepthVisMode::Colormap:
            CreateDepthColorMap(&normalizedDepth, &visualization);
            break;
            
        case DepthVisMode::Overlay:
        {
            // Blend depth with original image
            cv::Mat colorMap;
            CreateDepthColorMap(&normalizedDepth, &colorMap);
            
            cv::Mat frameResized;
            cv::resize(*frame, frameResized, cv::Size(Width, Height));
            
            if (frameResized.channels() == 3)
                cv::cvtColor(frameResized, frameResized, cv::COLOR_BGR2BGRA);
            
            cv::addWeighted(frameResized, 0.5, colorMap, 0.5, 0, visualization);
        }
            break;
    }
    
    // Resize to video size
    cv::resize(visualization, *depthResult, cv::Size(VideoSize.X, VideoSize.Y));
    
    // Update depth texture
    if (Depth_Texture2D && Depth_Texture2D->GetPlatformData())
    {
        FTexture2DMipMap& Mip = Depth_Texture2D->GetPlatformData()->Mips[0];
        void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
        
        if (Data)
        {
            FMemory::Memcpy(Data, depthResult->data, depthResult->total() * depthResult->elemSize());
            Mip.BulkData.Unlock();
            Depth_Texture2D->UpdateResource();
        }
    }
    
    // Update material
    if (Depth_Material)
    {
        Depth_Material->SetTextureParameterValue(FName("DepthTexture"), Depth_Texture2D);
        
        if (ImagePlateDepth)
        {
            ImagePlateDepth->MarkRenderStateDirty();
        }
    }
}

void AMidasDepth::CreateDepthColorMap(const cv::Mat* depthMap, cv::Mat* colorMap)
{
    // Apply colormap for better visualization
    cv::Mat colored;
    cv::applyColorMap(*depthMap, colored, cv::COLORMAP_TURBO);
    
    // Convert to BGRA
    cv::cvtColor(colored, *colorMap, cv::COLOR_BGR2BGRA);
}
