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
    AVAuthorizationStatus currentStatus = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    UE_LOG(LogTemp, Warning, TEXT("Camera auth status in InitializeCamera: %d"), (int)currentStatus);
#endif
    
    cap.open(CameraID);
    if (!cap.isOpened()) {
        UE_LOG(LogTemp, Error, TEXT("❌ Failed to open camera with ID %d"), CameraID);
        
#if PLATFORM_IOS
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
    
    cap.set(cv::CAP_PROP_BUFFERSIZE, 1);
    
    cv::Mat testFrame;
    bool frameCaptured = false;
    
    for (int attempt = 0; attempt < 5; attempt++) {
        if (cap.read(testFrame) && !testFrame.empty()) {
            UE_LOG(LogTemp, Warning, TEXT("✅ Test frame captured on attempt %d: %dx%d"),
                   attempt + 1, testFrame.cols, testFrame.rows);
            frameCaptured = true;
            
#if PLATFORM_IOS
            cv::rotate(testFrame, testFrame, cv::ROTATE_90_CLOCKWISE);
            VideoSize = FVector2D(testFrame.cols, testFrame.rows);
            UE_LOG(LogTemp, Warning, TEXT("Updated VideoSize for iOS after rotation: %fx%f"), VideoSize.X, VideoSize.Y);
#endif
            break;
        }
        
        FPlatformProcess::Sleep(0.1f);
        UE_LOG(LogTemp, Warning, TEXT("Test frame attempt %d failed, retrying..."), attempt + 1);
    }
    
    if (!frameCaptured) {
        UE_LOG(LogTemp, Error, TEXT("❌ Failed to capture test frame after 5 attempts, continuing anyway..."));
#if PLATFORM_IOS
        VideoSize = FVector2D(480, 360);
#endif
    }
    
    isStreamOpen = true;
    UE_LOG(LogTemp, Warning, TEXT("✅ isStreamOpen set to true"));
    
    ColorData.SetNumUninitialized(VideoSize.X * VideoSize.Y);
    cvSize = cv::Size(VideoSize.X, VideoSize.Y);
    cvMat = cv::Mat(cvSize, CV_8UC4, ColorData.GetData());
    
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
    
    for (TActorIterator<ACineCameraActor> It(GetWorld()); It; ++It)
    {
        CineCameraActor = *It;
        break;
    }
    
    APlayerStart* PlayerStart = nullptr;
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        PlayerStart = *It;
        break;
    }
    
    if (PlayerStart && ImagePlatePost)
    {
        FVector ForwardOffset = PlayerStart->GetActorForwardVector() * 25.0f;
        FVector PlacementLocation = PlayerStart->GetActorLocation() + ForwardOffset + FVector(0, 0, 100);
        FRotator PlacementRotation = PlayerStart->GetActorRotation();
        
        ImagePlatePost->SetWorldLocation(PlacementLocation);
        ImagePlatePost->SetWorldRotation(PlacementRotation);
        UE_LOG(LogTemp, Warning, TEXT("✅ ImagePlate positioned"));
    }
    
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
    
#if PLATFORM_MAC
    FString ModelPath = FPaths::ProjectConfigDir() / TEXT("Models/U2NET1.onnx");
    TArray<uint8> RawData;
    if (!FFileHelper::LoadFileToArray(RawData, *ModelPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load ONNX model from %s"), *ModelPath);
        return;
    }

    UNNEModelData* ModelData = NewObject<UNNEModelData>();
    ModelData->Init(TEXT("onnx"), RawData);

    CpuRuntime = UE::NNE::GetRuntime<INNERuntimeCPU>(TEXT("NNERuntimeORTCpu"));
    if (CpuRuntime.IsValid() && CpuRuntime->CanCreateModelCPU(ModelData) == INNERuntimeCPU::ECanCreateModelCPUStatus::Ok)
    {
        CpuModel = CpuRuntime->CreateModelCPU(ModelData);
        CpuModelInstance = CpuModel->CreateModelInstanceCPU();
        UE_LOG(LogTemp, Warning, TEXT("✅ ONNX model loaded"));
    }
#endif
    
    UE_LOG(LogTemp, Warning, TEXT("✅ Camera initialization complete"));
}

void AFurniLife::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (!isStreamOpen)
    {
        UE_LOG(LogTemp, Warning, TEXT("Stream not open"));
        return;
    }
    
    static double LastProcessTime = 0;
    double CurrentTime = GetWorld()->TimeSeconds;
    
    // Process at 15 FPS for better performance
    if ((CurrentTime - LastProcessTime) >= (1.0 / 15.0))
    {
        LastProcessTime = CurrentTime;
        
        bool bFrameRead = ReadFrame();
        
        // Log every 15 frames (once per second at 15fps)
        static int FrameCounter = 0;
        if (++FrameCounter % 15 == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("📹 Frame %d processed: %s"),
                   FrameCounter, bFrameRead ? TEXT("SUCCESS") : TEXT("FAILED"));
        }
    }
    
    // Debug input state
    static float DebugTimer = 0;
    DebugTimer += DeltaTime;
    if (DebugTimer > 1.0f)
    {
        DebugTimer = 0;
        
        APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
        if (PC)
        {
            APawn* Pawn = PC->GetPawn();
            UE_LOG(LogTemp, VeryVerbose, TEXT("Input Debug - Touch: %s, Pawn: %s"),
                   PC->bEnableTouchEvents ? TEXT("ON") : TEXT("OFF"),
                   Pawn ? *Pawn->GetName() : TEXT("NULL"));
        }
    }
}

bool AFurniLife::ReadFrame()
{
    static int EmptyFrameCount = 0;
    
    if (!cap.isOpened())
    {
        UE_LOG(LogTemp, Error, TEXT("Camera not open, attempting to reopen..."));
        cap.open(CameraID);
        return false;
    }
    
    // Clear buffer by reading multiple frames to get latest
    for (int i = 0; i < 2; i++) {
        cap.grab();
    }
    
    // Now read the actual frame
    if (!cap.read(frame))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to read frame"));
        return false;
    }
    
    if (frame.empty())
    {
        EmptyFrameCount++;
        if (EmptyFrameCount > 5)
        {
            UE_LOG(LogTemp, Error, TEXT("Too many empty frames, reinitializing camera"));
            cap.release();
            FPlatformProcess::Sleep(0.1f);
            cap.open(CameraID);
            EmptyFrameCount = 0;
        }
        return false;
    }
    
    EmptyFrameCount = 0;

#if PLATFORM_IOS
    cv::rotate(frame, frame, cv::ROTATE_90_CLOCKWISE);
    cv::flip(frame, frame, 0);
    
    FVector2D NewSize(frame.cols, frame.rows);
    if (NewSize != VideoSize)
    {
        UE_LOG(LogTemp, Warning, TEXT("Video size changed from %fx%f to %fx%f"),
               VideoSize.X, VideoSize.Y, NewSize.X, NewSize.Y);
        
        VideoSize = NewSize;
        
        if (Camera_Texture2D)
        {
            Camera_Texture2D->RemoveFromRoot();
            Camera_Texture2D = nullptr;
        }
        
        Camera_Texture2D = UTexture2D::CreateTransient(VideoSize.X, VideoSize.Y, PF_B8G8R8A8);
        if (!Camera_Texture2D)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to recreate texture"));
            return false;
        }
        
#if WITH_EDITORONLY_DATA
        Camera_Texture2D->MipGenSettings = TMGS_NoMipmaps;
#endif
        Camera_Texture2D->SRGB = false;
        Camera_Texture2D->AddToRoot();
        Camera_Texture2D->UpdateResource();
        
        ColorData.SetNumUninitialized(VideoSize.X * VideoSize.Y);
        
        AssignTextureToImagePlate();
        
        UE_LOG(LogTemp, Warning, TEXT("Texture recreated with size %fx%f"),
               VideoSize.X, VideoSize.Y);
    }
#endif
    
    ProcessInputForModel();
    RunModelInference();
    ApplySegmentationMask();
    
    cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA);
    
    if (frame.cols != VideoSize.X || frame.rows != VideoSize.Y)
    {
        cv::resize(frame, frame, cv::Size(VideoSize.X, VideoSize.Y));
    }
    
    int Width = VideoSize.X;
    int Height = VideoSize.Y;
    
    if (ColorData.Num() != Width * Height)
    {
        ColorData.SetNumUninitialized(Width * Height);
    }
    
    for (int y = 0; y < Height; ++y)
    {
        for (int x = 0; x < Width; ++x)
        {
            cv::Vec4b& srcPixel = frame.at<cv::Vec4b>(y, x);
            int index = y * Width + x;
            ColorData[index] = FColor(srcPixel[2], srcPixel[1], srcPixel[0], srcPixel[3]);
        }
    }
    
    UpdateTextureSafely();
    
    static bool bFirstFrame = true;
    if (bFirstFrame)
    {
        AssignTextureToImagePlate();
        bFirstFrame = false;
    }
    
    return true;
}

void AFurniLife::UpdateTextureSafely()
{
    if (!Camera_Texture2D || !Camera_Texture2D->GetResource())
    {
        UE_LOG(LogTemp, Error, TEXT("Texture or resource is null"));
        return;
    }
    
    static int UpdateCount = 0;
    if (++UpdateCount % 30 == 0)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("Texture update #%d, Size: %dx%d"),
               UpdateCount, (int)VideoSize.X, (int)VideoSize.Y);
    }
    
    TArray<uint8> TextureData;
    int32 Width = VideoSize.X;
    int32 Height = VideoSize.Y;
    TextureData.SetNumUninitialized(Width * Height * 4);
    
    for (int32 i = 0; i < ColorData.Num(); ++i)
    {
        int32 idx = i * 4;
        TextureData[idx + 0] = ColorData[i].B;
        TextureData[idx + 1] = ColorData[i].G;
        TextureData[idx + 2] = ColorData[i].R;
        TextureData[idx + 3] = ColorData[i].A;
    }
    
    struct FUpdateTextureData
    {
        UTexture2D* Texture;
        TArray<uint8> Data;
        int32 Width;
        int32 Height;
    };
    
    FUpdateTextureData* UpdateData = new FUpdateTextureData();
    UpdateData->Texture = Camera_Texture2D;
    UpdateData->Data = MoveTemp(TextureData);
    UpdateData->Width = Width;
    UpdateData->Height = Height;
    
    ENQUEUE_RENDER_COMMAND(SafeUpdateTexture)(
        [UpdateData](FRHICommandListImmediate& RHICmdList)
        {
            if (!UpdateData->Texture || !UpdateData->Texture->GetResource())
            {
                delete UpdateData;
                return;
            }
            
            FTexture2DResource* Resource = (FTexture2DResource*)UpdateData->Texture->GetResource();
            FRHITexture2D* Texture2DRHI = Resource->GetTexture2DRHI();
            
            if (!Texture2DRHI)
            {
                delete UpdateData;
                return;
            }
            
            if (Texture2DRHI->GetSizeX() != UpdateData->Width ||
                Texture2DRHI->GetSizeY() != UpdateData->Height)
            {
                UE_LOG(LogTemp, Error, TEXT("Texture dimension mismatch!"));
                delete UpdateData;
                return;
            }
            
            uint32 DestStride = 0;
            void* DestData = RHICmdList.LockTexture2D(
                Texture2DRHI,
                0,
                RLM_WriteOnly,
                DestStride,
                false
            );
            
            if (DestData)
            {
                uint32 SourceStride = UpdateData->Width * 4;
                
                if (DestStride == SourceStride)
                {
                    FMemory::Memcpy(DestData, UpdateData->Data.GetData(),
                                   UpdateData->Height * SourceStride);
                }
                else
                {
                    for (int32 y = 0; y < UpdateData->Height; ++y)
                    {
                        uint8* DestRow = (uint8*)DestData + (y * DestStride);
                        const uint8* SourceRow = UpdateData->Data.GetData() + (y * SourceStride);
                        FMemory::Memcpy(DestRow, SourceRow, SourceStride);
                    }
                }
                
                RHICmdList.UnlockTexture2D(Texture2DRHI, 0, false);
            }
            
            delete UpdateData;
        }
    );
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
        Camera_MatPost->SetScalarParameterValue(FName("UpdateTrigger"), FMath::FRand());
        
        UE_LOG(LogTemp, Warning, TEXT("✅ Texture parameters set on material"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Failed to get or create dynamic material"));
    }
    
    ImagePlatePost->SetVisibility(true);
    ImagePlatePost->SetHiddenInGame(false);
    ImagePlatePost->MarkRenderStateDirty();
    ImagePlatePost->MarkPackageDirty();
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

    const int srcStride = resized.step[0];
    const int srcCols = resized.cols;
    const int srcRows = resized.rows;

    for (int y = 0; y < srcRows; ++y)
    {
        const cv::Vec4b* srcRow = resized.ptr<cv::Vec4b>(y);
        uint8_t* dstRow = dstBase + y * dstBytesPerRow;
        for (int x = 0; x < srcCols; ++x)
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

    UE_LOG(LogTemp, VeryVerbose, TEXT("✅ CoreML inference success. Output size: %d"), (int)OutputBuffer.size());
#endif

#if PLATFORM_MAC
    const int Width = 320;
    const int Height = 320;
    if (!CpuModelInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("[Inference] CpuModelInstance is null."));
        return;
    }
    // Mac ONNX inference code...
#endif
}

void AFurniLife::ApplySegmentationMask()
{
    const int Width = 320;
    const int Height = 320;
    
#if PLATFORM_IOS
    if (OutputBuffer.empty()) {
        UE_LOG(LogTemp, Error, TEXT("OutputBuffer is empty"));
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
    
    // Use fixed threshold (adjust between 40-80 as needed)
    uchar threshold = 64;
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("Mask range: min=%f, max=%f, threshold=%d"),
           minVal, maxVal, threshold);
    
    for (int y = 0; y < frame.rows; ++y)
    {
        for (int x = 0; x < frame.cols; ++x)
        {
            uchar alpha = alphaMask.at<uchar>(y, x);
            cv::Vec4b& px = frame.at<cv::Vec4b>(y, x);
            
            if (alpha < threshold)
            {
                // Green screen for background
                px[0] = 0;   // B
                px[1] = 255; // G
                px[2] = 0;   // R
                px[3] = 255; // A
            }
            else
            {
                // Keep original pixel
                px[3] = 255;
            }
        }
    }
}

void AFurniLife::OnCameraFrameFromPixelBuffer(CVPixelBufferRef buffer)
{
    // Existing implementation...
    CVPixelBufferLockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
    int width = CVPixelBufferGetWidth(buffer);
    int height = CVPixelBufferGetHeight(buffer);
    uint8_t* base = (uint8_t*)CVPixelBufferGetBaseAddress(buffer);
    size_t stride = CVPixelBufferGetBytesPerRow(buffer);

    cv::Mat mat(height, width, CV_8UC4, base, stride);
    frame = mat.clone();
    CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);

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
    
    if (Camera_Texture2D) {
        UpdateTextureSafely();
        
        if (Camera_MatPost)
        {
            Camera_MatPost->SetTextureParameterValue(FName("Texture"), Camera_Texture2D);
        }
    }
}
