// FurniLife.cpp
#include "FurniLife.h"
#include "Misc/Paths.h"
#include "Engine/Texture2D.h"
#include "RenderUtils.h"
#include "RHICommandList.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Rendering/Texture2DResource.h"
#include "ImagePlateComponent.h"
#if PLATFORM_IOS
#include <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#include <fstream>
#import <UIKit/UIKit.h>
#import "CoreMLModelBridge.h"
#endif
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "CineCameraActor.h"
#include "Kismet/GameplayStatics.h"

AFurniLife* AFurniLife::CurrentInstance = nullptr;
ACineCameraActor* CineCameraActor = nullptr;

AFurniLife::AFurniLife(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    rootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    ImagePlatePost = CreateDefaultSubobject<UImagePlateComponent>(TEXT("ImagePlatePost"));

    Brightness = 0;
    Multiply = 1;
    CameraID = 0;
    VideoTrackID = 0;
    isStreamOpen = false;
    VideoSize = FVector2D(1920, 1080);
    RefreshRate = 30.0f;
}

void AFurniLife::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Warning, TEXT("Returned from Super::BeginPlay()"));
    
    FString MapName = GetWorld()->GetMapName();
    MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
    
    if (MapName.Contains("LoginLevel"))
    {
        UE_LOG(LogTemp, Log, TEXT("FurniLife actor in login level - skipping camera setup"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FurniLife starting in %s"), *MapName);
    AFurniLife::CurrentInstance = this;
    
    // ENSURE GAME INPUT MODE
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
        
        UE_LOG(LogTemp, Warning, TEXT("Input mode set to Game"));
    }

#if PLATFORM_IOS
    setenv("OPENCV_AVFOUNDATION_SKIP_AUTH", "0", 1);
    
    AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    UE_LOG(LogTemp, Warning, TEXT("Initial camera auth status: %d"), (int)status);
    
    if (status == AVAuthorizationStatusNotDetermined) {
        UE_LOG(LogTemp, Warning, TEXT("Requesting camera permission..."));
        [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
            UE_LOG(LogTemp, Warning, TEXT("Camera permission response: %s"), granted ? TEXT("GRANTED") : TEXT("DENIED"));
            if (granted) {
                // Use AsyncTask instead of dispatch_async for Unreal
                AsyncTask(ENamedThreads::GameThread, [this]() {
                    InitializeCamera();
                });
            }
        }];
    } else if (status == AVAuthorizationStatusAuthorized) {
        UE_LOG(LogTemp, Warning, TEXT("Camera already authorized"));
        InitializeCamera();
    } else {
        UE_LOG(LogTemp, Error, TEXT("Camera permission denied or restricted. Status: %d"), (int)status);
    }
#else
    InitializeCamera();
#endif
}

void AFurniLife::InitializeCamera()
{
    UE_LOG(LogTemp, Warning, TEXT("InitializeCamera called"));
    
#if PLATFORM_IOS
    // Check current authorization status again
    AVAuthorizationStatus currentStatus = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    UE_LOG(LogTemp, Warning, TEXT("Camera auth status in InitializeCamera: %d"), (int)currentStatus);
#endif
    
    // Open camera
    cap.open(CameraID);
    if (!cap.isOpened()) {
        UE_LOG(LogTemp, Error, TEXT("❌ Failed to open camera with ID %d"), CameraID);
        
#if PLATFORM_IOS
        // Try different camera IDs on iOS
        for (int i = 0; i < 3; i++) {
            UE_LOG(LogTemp, Warning, TEXT("Trying camera ID %d..."), i);
            cap.open(i);
            if (cap.isOpened()) {
                UE_LOG(LogTemp, Warning, TEXT("✅ Camera opened with ID %d"), i);
                CameraID = i;
                break;
            }
        }
        
        if (!cap.isOpened()) {
            UE_LOG(LogTemp, Error, TEXT("❌ Failed to open any camera"));
            return;
        }
#else
        return;
#endif
    } else {
        UE_LOG(LogTemp, Warning, TEXT("✅ Camera opened successfully with ID %d"), CameraID);
    }
    
    // Set buffer size
    cap.set(cv::CAP_PROP_BUFFERSIZE, 1);
    
    // Check if camera is really working
    cv::Mat testFrame;
    if (cap.read(testFrame)) {
        UE_LOG(LogTemp, Warning, TEXT("✅ Test frame captured: %dx%d"), testFrame.cols, testFrame.rows);
        
#if PLATFORM_IOS
        // Update VideoSize based on actual camera dimensions after rotation
        VideoSize = FVector2D(testFrame.rows, testFrame.cols);  // Swapped for rotation
        UE_LOG(LogTemp, Warning, TEXT("Updated VideoSize for iOS: %fx%f"), VideoSize.X, VideoSize.Y);
#endif
    } else {
        UE_LOG(LogTemp, Error, TEXT("❌ Failed to capture test frame"));
//        return;
    }
    
    isStreamOpen = true;
    UE_LOG(LogTemp, Warning, TEXT("✅ isStreamOpen set to true"));
    
    // Initialize buffers with updated size
    ColorData.SetNumUninitialized(VideoSize.X * VideoSize.Y);
    cvSize = cv::Size(VideoSize.X, VideoSize.Y);
    cvMat = cv::Mat(cvSize, CV_8UC4, ColorData.GetData());
    
    // Create textures
    Camera_Texture2D = UTexture2D::CreateTransient(VideoSize.X, VideoSize.Y, PF_B8G8R8A8);
    VideoMask_Texture2D = UTexture2D::CreateTransient(VideoSize.X, VideoSize.Y, PF_G8);
    
    if (!Camera_Texture2D) {
        UE_LOG(LogTemp, Error, TEXT("❌ Failed to create Camera_Texture2D"));
        return;
    }
    
#if WITH_EDITORONLY_DATA
    Camera_Texture2D->MipGenSettings = TMGS_NoMipmaps;
#endif
    
    Camera_Texture2D->SRGB = false;
    Camera_Texture2D->AddToRoot();
    VideoMask_Texture2D->SRGB = false;
    Camera_Texture2D->UpdateResource();
    
    UE_LOG(LogTemp, Warning, TEXT("✅ Textures created"));
    
    // Rest of your setup code...
    
    UE_LOG(LogTemp, Warning, TEXT("✅ Camera initialization complete"));
    
    // Position ImagePlate
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        APlayerStart* PlayerStart = *It;
        if (PlayerStart && ImagePlatePost)
        {
            FVector ForwardOffset = PlayerStart->GetActorForwardVector() * 100.0f;
            FVector PlacementLocation = PlayerStart->GetActorLocation() + ForwardOffset + FVector(0, 0, 100);
            FRotator PlacementRotation = PlayerStart->GetActorRotation();
            
            ImagePlatePost->SetWorldLocation(PlacementLocation);
            ImagePlatePost->SetWorldRotation(PlacementRotation);
            ImagePlatePost->SetWorldScale3D(FVector(4, 3, 1));
            
            AssignTextureToImagePlate();
            UE_LOG(LogTemp, Warning, TEXT("✅ ImagePlate positioned and texture assigned"));
        }
        break;
    }
        
    // Load CoreML model
    #if PLATFORM_IOS
    NSString* ModelPath = [[NSBundle mainBundle] pathForResource:@"u2net" ofType:@"mlmodelc"];
    if (!ModelPath) {
        UE_LOG(LogTemp, Error, TEXT("❌ Could not find u2net.mlmodelc in bundle."));
        return;
    }
    
    if (!GetSharedBridge()->LoadModel([ModelPath UTF8String])) {
        UE_LOG(LogTemp, Error, TEXT("❌ Failed to load CoreML model."));
        return;
    }
    UE_LOG(LogTemp, Warning, TEXT("✅ CoreML model loaded"));
  #endif
}

void AFurniLife::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (!isStreamOpen)
        return;
    
    static float TimeSinceLastFrame = 0;
    TimeSinceLastFrame += DeltaTime;
    
    // Process at 15 FPS to avoid overloading
    if (TimeSinceLastFrame >= 1.0f / 15.0f)
    {
        TimeSinceLastFrame = 0;
        ReadFrame();
        
        // Log occasionally
        static int FrameCount = 0;
        if (++FrameCount % 30 == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("Frame %d processed"), FrameCount);
        }
    }
    //return;
}

bool AFurniLife::ReadFrame()
{
//    if (!cap.isOpened())
//    {
//        cap.open(CameraID);
//        return false;
//    }
//    
//    if (!cap.read(frame) || frame.empty())
//    {
//        return false;
//    }
//
//#if PLATFORM_IOS
//    cv::rotate(frame, frame, cv::ROTATE_90_COUNTERCLOCKWISE);
////    cv::flip(frame, frame, 1);
//#endif
    
    // KEEP SEGMENTATION DISABLED UNTIL VIDEO WORKS
    bool bEnableSegmentation = false;
    
    if (bEnableSegmentation)
    {
        ProcessInputForModel();
        RunModelInference();
        ApplySegmentationMask();
    }
    
    // Convert to BGRA
//    if (frame.channels() == 3)
//    {
//        cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA);
//    }
    
    // Ensure correct size
//    if (frame.cols != VideoSize.X || frame.rows != VideoSize.Y)
//    {
//        cv::resize(frame, frame, cv::Size(VideoSize.X, VideoSize.Y));
//    }
//    
//    // Direct texture update without render commands
//    if (Camera_Texture2D && Camera_Texture2D->GetPlatformData())
//    {
//        FTexture2DMipMap& Mip = Camera_Texture2D->GetPlatformData()->Mips[0];
//        void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
//        
//        if (Data)
//        {
//            uint8* DestPtr = (uint8*)Data;
//            for (int y = 0; y < frame.rows; ++y)
//            {
//                const cv::Vec4b* srcRow = frame.ptr<cv::Vec4b>(y);
//                for (int x = 0; x < frame.cols; ++x)
//                {
//                    const cv::Vec4b& pixel = srcRow[x];
//                    *DestPtr++ = pixel[0]; // B
//                    *DestPtr++ = pixel[1]; // G
//                    *DestPtr++ = pixel[2]; // R
//                    *DestPtr++ = 255;      // A - force opaque
//                }
//            }
//            
//            Mip.BulkData.Unlock();
//            Camera_Texture2D->UpdateResource();
//        }
//    }
    
    // Update material
//    static bool bFirstFrame = true;
//    if (bFirstFrame)
//    {
//        AssignTextureToImagePlate();
//        bFirstFrame = false;
//    }
    
    return true;
}

void AFurniLife::UpdateTextureSafely()
{
    // Not used anymore - direct update in ReadFrame instead
}

void AFurniLife::AssignTextureToImagePlate()
{
    if (!ImagePlatePost || !Camera_Texture2D)
    {
        UE_LOG(LogTemp, Warning, TEXT("ImagePlatePost or Camera_Texture2D is null"));
        return;
    }
    
    UMaterialInterface* CurrentMaterial = ImagePlatePost->GetMaterial(0);
    
    if (!CurrentMaterial)
    {
        UMaterial* UnlitMaterial = LoadObject<UMaterial>(nullptr,
            TEXT("/Engine/EngineMaterials/UnlitMaterial.UnlitMaterial"));
        
        if (UnlitMaterial)
        {
            Camera_MatPost = UMaterialInstanceDynamic::Create(UnlitMaterial, this);
            ImagePlatePost->SetMaterial(0, Camera_MatPost);
            UE_LOG(LogTemp, Warning, TEXT("Created unlit material"));
        }
    }
    else
    {
        Camera_MatPost = Cast<UMaterialInstanceDynamic>(CurrentMaterial);
        if (!Camera_MatPost)
        {
            Camera_MatPost = UMaterialInstanceDynamic::Create(CurrentMaterial, this);
            if (Camera_MatPost)
            {
                ImagePlatePost->SetMaterial(0, Camera_MatPost);
            }
        }
    }
    
    if (Camera_MatPost)
    {
        Camera_MatPost->SetTextureParameterValue(FName("Texture"), Camera_Texture2D);
        Camera_MatPost->SetTextureParameterValue(FName("BaseTexture"), Camera_Texture2D);
        Camera_MatPost->SetTextureParameterValue(FName("MainTexture"), Camera_Texture2D);
        
        UE_LOG(LogTemp, Warning, TEXT("✅ Texture parameters set on material"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Failed to get or create dynamic material"));
    }
    
    ImagePlatePost->SetVisibility(true);
    ImagePlatePost->SetHiddenInGame(false);
    ImagePlatePost->MarkRenderStateDirty();
}

void AFurniLife::ProcessInputForModel()
{
    cv::resize(frame, resized, cv::Size(320, 320));
    cv::cvtColor(resized, resized, cv::COLOR_BGR2RGB);
    resized.convertTo(resized, CV_32FC3, 1.0 / 255.0);
}

void AFurniLife::RunModelInference()
{
#if PLATFORM_IOS
    const int Width = 320, Height = 320;

    if (resized.channels() == 3) {
        cv::cvtColor(resized, resized, cv::COLOR_BGR2RGBA);
    } else if (resized.channels() == 4) {
        cv::cvtColor(resized, resized, cv::COLOR_BGRA2RGBA);
    }

    CVPixelBufferRef pixelBuffer = nullptr;
    NSDictionary* attrs = @{
        (__bridge NSString*)kCVPixelBufferCGImageCompatibilityKey: @YES,
        (__bridge NSString*)kCVPixelBufferCGBitmapContextCompatibilityKey: @YES
    };
    CVReturn status = CVPixelBufferCreate(kCFAllocatorDefault,
                                          Width,
                                          Height,
                                          kCVPixelFormatType_32BGRA,
                                          (__bridge CFDictionaryRef)attrs,
                                          &pixelBuffer);

    if (status != kCVReturnSuccess || pixelBuffer == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("❌ Failed to create CVPixelBuffer"));
        return;
    }

    CVPixelBufferLockBaseAddress(pixelBuffer, 0);
    uint8_t* dstBase = reinterpret_cast<uint8_t*>(CVPixelBufferGetBaseAddress(pixelBuffer));
    const size_t dstBytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);

    if (!dstBase || !resized.data) {
        UE_LOG(LogTemp, Error, TEXT("❌ PixelBuffer or resized image data is NULL"));
        CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
        CVPixelBufferRelease(pixelBuffer);
        return;
    }

    for (int y = 0; y < Height; ++y)
    {
        const cv::Vec4b* srcRow = resized.ptr<cv::Vec4b>(y);
        uint8_t* dstRow = dstBase + y * dstBytesPerRow;
        for (int x = 0; x < Width; ++x)
        {
            const cv::Vec4b& px = srcRow[x];
            dstRow[x * 4 + 0] = px[0];
            dstRow[x * 4 + 1] = px[1];
            dstRow[x * 4 + 2] = px[2];
            dstRow[x * 4 + 3] = px[3];
        }
    }

    CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);

    std::vector<float> OutputData;
    if (!GetSharedBridge() || !GetSharedBridge()->RunModel(pixelBuffer, OutputData)) {
        UE_LOG(LogTemp, Error, TEXT("❌ CoreML inference failed."));
        CVPixelBufferRelease(pixelBuffer);
        return;
    }

    OutputBuffer = std::move(OutputData);
    CVPixelBufferRelease(pixelBuffer);
#endif

#if PLATFORM_MAC
    // Mac ONNX inference code...
#endif
}

void AFurniLife::ApplySegmentationMask()
{
    const int Width = 320;
    const int Height = 320;
    
#if PLATFORM_IOS
    if (OutputBuffer.empty()) {
        return;
    }
    cv::Mat mask(Height, Width, CV_32FC1, OutputBuffer.data());
#endif
    
#if PLATFORM_MAC
    const float* MaskOutput = OutputTensors.Last().GetData();
    cv::Mat mask(Height, Width, CV_32FC1, (void*)MaskOutput);
#endif

    // Normalize mask to 0-255 range
    double minVal, maxVal;
    cv::minMaxLoc(mask, &minVal, &maxVal);
    
    cv::Mat normalized;
    if (maxVal > 0)
    {
        mask.convertTo(normalized, CV_8UC1, 255.0 / maxVal);
    }
    else
    {
        normalized = cv::Mat::zeros(Height, Width, CV_8UC1);
    }
    
    cv::resize(normalized, alphaMask, cv::Size(VideoSize.X, VideoSize.Y));
    
    if (frame.channels() == 3)
    {
        cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA);
    }
    
    // Use threshold (adjust between 40-80 as needed)
    uchar threshold = 64;
    
    for (int y = 0; y < frame.rows; ++y)
    {
        for (int x = 0; x < frame.cols; ++x)
        {
            uchar alpha = alphaMask.at<uchar>(y, x);
            cv::Vec4b& px = frame.at<cv::Vec4b>(y, x);
            
            if (alpha < threshold)
            {
                px[0] = 0;   // B
                px[1] = 255; // G
                px[2] = 0;   // R
                px[3] = 255; // A
            }
            else
            {
                px[3] = 255;
            }
        }
    }
}

void AFurniLife::OnCameraFrameFromPixelBuffer(CVPixelBufferRef buffer)
{
    CVPixelBufferLockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
    int width = CVPixelBufferGetWidth(buffer);
    int height = CVPixelBufferGetHeight(buffer);
    uint8_t* base = (uint8_t*)CVPixelBufferGetBaseAddress(buffer);
    size_t stride = CVPixelBufferGetBytesPerRow(buffer);

    cv::Mat mat(height, width, CV_8UC4, base, stride);
    frame = mat.clone();
    CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);

    // Process similar to ReadFrame
    ProcessInputForModel();
    RunModelInference();
    ApplySegmentationMask();
    
    cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA);
    
    int Width = frame.cols;
    int Height = frame.rows;
    VideoSize = FVector2D(Width, Height);
    
    ColorData.SetNumUninitialized(Width * Height);
    
    for (int y = 0; y < Height; ++y)
    {
        for (int x = 0; x < Width; ++x)
        {
            cv::Vec4b& srcPixel = frame.at<cv::Vec4b>(y, x);
            int index = y * Width + x;
            ColorData[index] = FColor(srcPixel[2], srcPixel[1], srcPixel[0], srcPixel[3]);
        }
    }
    
    if (Camera_Texture2D && Camera_Texture2D->GetPlatformData())
    {
        FTexture2DMipMap& Mip = Camera_Texture2D->GetPlatformData()->Mips[0];
        void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
        
        if (Data)
        {
            FMemory::Memcpy(Data, ColorData.GetData(), ColorData.Num() * sizeof(FColor));
            Mip.BulkData.Unlock();
            Camera_Texture2D->UpdateResource();
        }
        
        if (Camera_MatPost)
        {
            Camera_MatPost->SetTextureParameterValue(FName("Texture"), Camera_Texture2D);
        }
    }
}
