// FurniLife.cpp
#include "FurniLife.h"
#include "Misc/Paths.h"
#include "Engine/Texture2D.h"
#include "RenderUtils.h"
#include "RHICommandList.h"
#include "Materials/Material.h"
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
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "CineCameraActor.h"
#include "Kismet/GameplayStatics.h"

AFurniLife* AFurniLife::CurrentInstance = nullptr;
ACineCameraActor* CineCameraActor = nullptr;

AFurniLife::AFurniLife(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    
    // Create root component
    rootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(rootComp);
    
    // DON'T create ImagePlate in constructor - do it dynamically
    ImagePlatePost = nullptr;

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
    
    // Open camera
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
    cap.set(cv::CAP_PROP_FPS, 30);
    
#if PLATFORM_IOS
    VideoSize = FVector2D(480, 360);
    UE_LOG(LogTemp, Warning, TEXT("Using default VideoSize for iOS: %fx%f"), VideoSize.X, VideoSize.Y);
#else
    cv::Mat testFrame;
    if (cap.read(testFrame) && !testFrame.empty()) {
        VideoSize = FVector2D(testFrame.cols, testFrame.rows);
        UE_LOG(LogTemp, Warning, TEXT("Test frame size: %fx%f"), VideoSize.X, VideoSize.Y);
    } else {
        VideoSize = FVector2D(640, 480);
        UE_LOG(LogTemp, Warning, TEXT("Using default VideoSize"));
    }
#endif
    
    // ALWAYS set isStreamOpen to true if camera opened
    isStreamOpen = true;
    UE_LOG(LogTemp, Warning, TEXT("✅ isStreamOpen set to true"));
    
    // Initialize buffers
    ColorData.SetNumUninitialized(VideoSize.X * VideoSize.Y);
    cvSize = cv::Size(VideoSize.X, VideoSize.Y);
    cvMat = cv::Mat(cvSize, CV_8UC4, ColorData.GetData());
    
    // Create textures
    Camera_Texture2D = UTexture2D::CreateTransient(VideoSize.X, VideoSize.Y, PF_B8G8R8A8);
    VideoMask_Texture2D = UTexture2D::CreateTransient(VideoSize.X, VideoSize.Y, PF_G8);
    
    if (!Camera_Texture2D) {
        UE_LOG(LogTemp, Error, TEXT("❌ Failed to create Camera_Texture2D"));
        isStreamOpen = false;
        return;
    }
    
#if WITH_EDITORONLY_DATA
    Camera_Texture2D->MipGenSettings = TMGS_NoMipmaps;
#endif
    
    Camera_Texture2D->SRGB = true;
    Camera_Texture2D->AddToRoot();
    VideoMask_Texture2D->SRGB = true;
    Camera_Texture2D->UpdateResource();
    
    UE_LOG(LogTemp, Warning, TEXT("✅ Textures created"));
    
    // Create ImagePlate dynamically
    if (!ImagePlatePost)
    {
        ImagePlatePost = NewObject<UImagePlateComponent>(this, TEXT("DynamicImagePlate"));
        ImagePlatePost->SetupAttachment(RootComponent);
        ImagePlatePost->RegisterComponent();
        
        ImagePlatePost->bAutoActivate = true;
        ImagePlatePost->SetVisibility(true);
        ImagePlatePost->SetHiddenInGame(false);
        
        UE_LOG(LogTemp, Warning, TEXT("✅ Created ImagePlate dynamically"));
    }
    
    // Position ImagePlate properly
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        APlayerStart* PlayerStart = *It;
        if (PlayerStart && ImagePlatePost)
        {
            FVector ForwardOffset = PlayerStart->GetActorForwardVector() * 300.0f;
            FVector PlacementLocation = PlayerStart->GetActorLocation() + ForwardOffset + FVector(0, 0, 100);
            
            ImagePlatePost->SetWorldLocation(PlacementLocation);
            ImagePlatePost->SetWorldRotation(PlayerStart->GetActorRotation());
            
            // Use SetRelativeScale3D to avoid scale issues
            ImagePlatePost->SetRelativeScale3D(FVector(4.0f, 3.0f, 1.0f));
            
            UE_LOG(LogTemp, Warning, TEXT("ImagePlate positioned at: %s with scale: %s"),
                   *PlacementLocation.ToString(),
                   *ImagePlatePost->GetComponentScale().ToString());
            
            // Load your M_CameraFeed material
            UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Game/FurniLife/Camera_Post.Camera_Post"));
            
            if (BaseMaterial)
            {
                Camera_MatPost = UMaterialInstanceDynamic::Create(BaseMaterial, this);
                ImagePlatePost->SetMaterial(0, Camera_MatPost);
                
                if (Camera_Texture2D)
                {
                    // Set the texture parameter - use the exact name from your material
                    Camera_MatPost->SetTextureParameterValue(FName("Texture"), Camera_Texture2D);
                }
                
                UE_LOG(LogTemp, Warning, TEXT("✅ M_CameraFeed material loaded and texture set"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("❌ Failed to load M_CameraFeed material - check the path"));
            }
        }
        break;
    }
    
#if PLATFORM_IOS
    // Load CoreML model
    NSString* ModelPath = [[NSBundle mainBundle] pathForResource:@"u2net" ofType:@"mlmodelc"];
    if (!ModelPath) {
        UE_LOG(LogTemp, Error, TEXT("❌ Could not find u2net.mlmodelc in bundle."));
    } else if (!GetSharedBridge()->LoadModel([ModelPath UTF8String])) {
        UE_LOG(LogTemp, Error, TEXT("❌ Failed to load CoreML model."));
    } else {
        UE_LOG(LogTemp, Warning, TEXT("✅ CoreML model loaded"));
    }
#endif
    
    UE_LOG(LogTemp, Warning, TEXT("✅ Camera initialization complete"));
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
        bool success = ReadFrame();
        
        // Enhanced debug logging
        static int FrameCount = 0;
        if (++FrameCount % 30 == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("Frame %d: %s, Texture: %s, Material: %s, ImagePlate: %s"),
                FrameCount,
                success ? TEXT("OK") : TEXT("FAILED"),
                Camera_Texture2D ? TEXT("Valid") : TEXT("NULL"),
                Camera_MatPost ? TEXT("Valid") : TEXT("NULL"),
                (ImagePlatePost && ImagePlatePost->IsVisible()) ? TEXT("Visible") : TEXT("Not Visible"));
        }
    }
}

bool AFurniLife::ReadFrame()
{
    if (!cap.isOpened())
    {
        cap.open(CameraID);
        return false;
    }
    
    if (!cap.read(frame) || frame.empty())
    {
        return false;
    }

#if PLATFORM_IOS
    cv::rotate(frame, frame, cv::ROTATE_90_CLOCKWISE);
//    cv::flip(frame, frame, 0);  // Horizontal flip to fix orientation
    
    static bool bFirstValidFrame = true;
    if (bFirstValidFrame && !frame.empty())
    {
        VideoSize = FVector2D(frame.cols, frame.rows);
        UE_LOG(LogTemp, Warning, TEXT("Actual frame size after rotation: %fx%f"), VideoSize.X, VideoSize.Y);
        
        if (Camera_Texture2D && (Camera_Texture2D->GetSizeX() != VideoSize.X || Camera_Texture2D->GetSizeY() != VideoSize.Y))
        {
            Camera_Texture2D->RemoveFromRoot();
            Camera_Texture2D = UTexture2D::CreateTransient(VideoSize.X, VideoSize.Y, PF_B8G8R8A8);
            Camera_Texture2D->SRGB = false;
            Camera_Texture2D->AddToRoot();
            Camera_Texture2D->UpdateResource();
            
            ColorData.SetNumUninitialized(VideoSize.X * VideoSize.Y);
            
            if (Camera_MatPost)
            {
                Camera_MatPost->SetTextureParameterValue(FName("Texture"), Camera_Texture2D);
            }
        }
        
        bFirstValidFrame = false;
    }
#endif
    
    // TEMPORARILY DISABLE segmentation to test video first
    bool bEnableSegmentation = true;  // Set to true once video works
    
    if (bEnableSegmentation)
    {
        ProcessInputForModel();
        RunModelInference();
        ApplySegmentationMask();
    }
    
    // Convert to BGRA
    if (frame.channels() == 3)
    {
        cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA);
    }
    
    // Ensure correct size
    if (frame.cols != VideoSize.X || frame.rows != VideoSize.Y)
    {
        cv::resize(frame, frame, cv::Size(VideoSize.X, VideoSize.Y));
    }
    
    // Direct texture update
    if (Camera_Texture2D && Camera_Texture2D->GetPlatformData())
    {
        FTexture2DMipMap& Mip = Camera_Texture2D->GetPlatformData()->Mips[0];
        void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
        
        if (Data)
        {
            uint8* DestPtr = (uint8*)Data;
            for (int y = 0; y < frame.rows; ++y)
            {
                const cv::Vec4b* srcRow = frame.ptr<cv::Vec4b>(y);
                for (int x = 0; x < frame.cols; ++x)
                {
                    const cv::Vec4b& pixel = srcRow[x];
                    *DestPtr++ = pixel[0]; // B
                    *DestPtr++ = pixel[1]; // G
                    *DestPtr++ = pixel[2]; // R
                    *DestPtr++ = 255;      // A - force opaque
                }
            }
            
            Mip.BulkData.Unlock();
            Camera_Texture2D->UpdateResource();
        }
    }
    
    // Force material update every frame
    if (Camera_MatPost && Camera_Texture2D)
    {
        Camera_MatPost->SetTextureParameterValue(FName("Texture"), Camera_Texture2D);
        Camera_MatPost->SetTextureParameterValue(FName("Texture Render Target"), Camera_Texture2D);
        
        if (ImagePlatePost)
        {
            ImagePlatePost->MarkRenderStateDirty();
        }
    }
    
    return true;
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
    // Mac ONNX inference code would go here
#endif
}

//void AFurniLife::ApplySegmentationMask()
//{
//    const int Width = 320;
//    const int Height = 320;
//    
//#if PLATFORM_IOS
//    if (OutputBuffer.empty()) {
//        return;
//    }
//    cv::Mat mask(Height, Width, CV_32FC1, OutputBuffer.data());
//#endif
//    
//#if PLATFORM_MAC
//    const float* MaskOutput = OutputTensors.Last().GetData();
//    cv::Mat mask(Height, Width, CV_32FC1, (void*)MaskOutput);
//#endif
//
//    // Normalize mask to 0-255 range
//    double minVal, maxVal;
//    cv::minMaxLoc(mask, &minVal, &maxVal);
//    
//    cv::Mat normalized;
//    if (maxVal > 0)
//    {
//        mask.convertTo(normalized, CV_8UC1, 255.0 / maxVal);
//    }
//    else
//    {
//        normalized = cv::Mat::zeros(Height, Width, CV_8UC1);
//    }
//    
//    cv::resize(normalized, alphaMask, cv::Size(VideoSize.X, VideoSize.Y));
//    
//    if (frame.channels() == 3)
//    {
//        cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA);
//    }
//    
//    // Adjust threshold as needed (40-80 range)
//    uchar threshold = 64;
//    
//    for (int y = 0; y < frame.rows; ++y)
//    {
//        for (int x = 0; x < frame.cols; ++x)
//        {
//            uchar alpha = alphaMask.at<uchar>(y, x);
//            cv::Vec4b& px = frame.at<cv::Vec4b>(y, x);
//            
//            if (alpha < threshold)
//            {
//                px[0] = 0;   // B
//                px[1] = 255; // G
//                px[2] = 0;   // R
//                px[3] = 255; // A
//            }
//            else
//            {
//                px[3] = 255; // Keep original with full alpha
//            }
//        }
//    }
//}

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
    
    // SMART THRESHOLD CALCULATION
    uchar threshold = CalculateSmartThreshold(normalized);
    
    // Apply temporal smoothing to avoid flickering
    threshold = ApplyTemporalSmoothing(threshold);
    
    cv::resize(normalized, alphaMask, cv::Size(VideoSize.X, VideoSize.Y));
    
    if (frame.channels() == 3)
    {
        cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA);
    }
    
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
                px[3] = 255; // Keep original with full alpha
            }
        }
    }
}

// Method 1: Otsu's Method (Best for bimodal distributions)
uchar AFurniLife::CalculateOtsuThreshold(const cv::Mat& mask)
{
    cv::Mat binary;
    double otsuThreshold = cv::threshold(mask, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    
    // Otsu often needs adjustment for segmentation masks
    // Apply a bias factor based on mask characteristics
    float biasFactor = 0.8f; // Slightly lower to include more foreground
    
    return static_cast<uchar>(otsuThreshold * biasFactor);
}

// Method 2: Percentile-based (Robust to outliers)
uchar AFurniLife::CalculatePercentileThreshold(const cv::Mat& mask)
{
    // Flatten the mask to 1D for sorting
    cv::Mat flat = mask.reshape(1, mask.total());
    cv::Mat sorted;
    cv::sort(flat, sorted, cv::SORT_ASCENDING);
    
    // Use the value at 30th percentile as threshold
    // This assumes ~30% of image is background
    float percentile = 0.3f; // Adjustable based on typical scene composition
    int index = static_cast<int>(sorted.total() * percentile);
    
    return sorted.at<uchar>(index);
}

// Method 3: Histogram Analysis (Most comprehensive)
uchar AFurniLife::CalculateHistogramThreshold(const cv::Mat& mask)
{
    // Calculate histogram
    int histSize = 256;
    float range[] = {0, 256};
    const float* histRange = {range};
    cv::Mat hist;
    cv::calcHist(&mask, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange);
    
    // Find the valley between two peaks (bimodal distribution)
    // Smooth histogram first to reduce noise
    cv::GaussianBlur(hist, hist, cv::Size(1, 5), 0);
    
    // Find peaks
    int peak1 = 0, peak2 = 255;
    float maxVal1 = 0, maxVal2 = 0;
    
    // Find first peak (background)
    for (int i = 0; i < 128; i++)
    {
        float val = hist.at<float>(i);
        if (val > maxVal1)
        {
            maxVal1 = val;
            peak1 = i;
        }
    }
    
    // Find second peak (foreground)
    for (int i = 128; i < 256; i++)
    {
        float val = hist.at<float>(i);
        if (val > maxVal2)
        {
            maxVal2 = val;
            peak2 = i;
        }
    }
    
    // Find valley between peaks
    int valley = peak1;
    float minVal = hist.at<float>(peak1);
    
    for (int i = peak1; i <= peak2; i++)
    {
        float val = hist.at<float>(i);
        if (val < minVal)
        {
            minVal = val;
            valley = i;
        }
    }
    
    return static_cast<uchar>(valley);
}

// Method 4: Adaptive Mean-based
uchar AFurniLife::CalculateAdaptiveThreshold(const cv::Mat& mask)
{
    // Calculate mean and standard deviation
    cv::Scalar mean, stddev;
    cv::meanStdDev(mask, mean, stddev);
    
    // For segmentation masks, pixels tend to cluster at extremes
    // Use mean - k*stddev for conservative foreground detection
    float k = 0.5f; // Tunable parameter
    
    float threshold = mean[0] - k * stddev[0];
    
    // Clamp to valid range
    threshold = std::max(10.0f, std::min(245.0f, threshold));
    
    return static_cast<uchar>(threshold);
}

// Main smart threshold function combining multiple methods
uchar AFurniLife::CalculateSmartThreshold(const cv::Mat& mask)
{
    // Method selection based on mask characteristics
    cv::Scalar mean, stddev;
    cv::meanStdDev(mask, mean, stddev);
    
    float coefficientOfVariation = stddev[0] / (mean[0] + 1e-6);
    
    uchar threshold;
    
    if (coefficientOfVariation > 0.8f)
    {
        // High variance - likely good separation
        // Use Otsu's method
        threshold = CalculateOtsuThreshold(mask);
        
        if (bDebugMode)
        {
            UE_LOG(LogTemp, Warning, TEXT("Using Otsu threshold: %d"), threshold);
        }
    }
    else if (coefficientOfVariation < 0.3f)
    {
        // Low variance - poor separation
        // Use percentile method
        threshold = CalculatePercentileThreshold(mask);
        
        if (bDebugMode)
        {
            UE_LOG(LogTemp, Warning, TEXT("Using Percentile threshold: %d"), threshold);
        }
    }
    else
    {
        // Medium variance - use histogram analysis
        threshold = CalculateHistogramThreshold(mask);
        
        if (bDebugMode)
        {
            UE_LOG(LogTemp, Warning, TEXT("Using Histogram threshold: %d"), threshold);
        }
    }
    
    // Confidence-based adjustment
    float confidence = CalculateSegmentationConfidence(mask, threshold);
    
    if (confidence < 0.6f)
    {
        // Low confidence - use fallback adaptive method
        uchar adaptiveThreshold = CalculateAdaptiveThreshold(mask);
        threshold = static_cast<uchar>(0.7f * threshold + 0.3f * adaptiveThreshold);
        
        if (bDebugMode)
        {
            UE_LOG(LogTemp, Warning, TEXT("Low confidence %.2f, blending with adaptive: %d"),
                   confidence, threshold);
        }
    }
    
    return threshold;
}

// Calculate confidence in the segmentation
float AFurniLife::CalculateSegmentationConfidence(const cv::Mat& mask, uchar threshold)
{
    int foregroundCount = 0;
    int backgroundCount = 0;
    int uncertainCount = 0;
    
    // Define uncertainty band around threshold
    int uncertaintyBand = 20;
    
    for (int y = 0; y < mask.rows; ++y)
    {
        for (int x = 0; x < mask.cols; ++x)
        {
            uchar val = mask.at<uchar>(y, x);
            
            if (val > threshold + uncertaintyBand)
            {
                foregroundCount++;
            }
            else if (val < threshold - uncertaintyBand)
            {
                backgroundCount++;
            }
            else
            {
                uncertainCount++;
            }
        }
    }
    
    int totalPixels = mask.rows * mask.cols;
    float uncertainRatio = static_cast<float>(uncertainCount) / totalPixels;
    
    // High confidence when few pixels are in uncertainty band
    float confidence = 1.0f - uncertainRatio;
    
    // Also check if we have reasonable foreground/background distribution
    float foregroundRatio = static_cast<float>(foregroundCount) / totalPixels;
    
    if (foregroundRatio < 0.05f || foregroundRatio > 0.95f)
    {
        // Extreme ratios indicate poor segmentation
        confidence *= 0.5f;
    }
    
    return confidence;
}

// Temporal smoothing to prevent threshold flickering
uchar AFurniLife::ApplyTemporalSmoothing(uchar currentThreshold)
{
    // Keep history of last N thresholds
    static std::deque<uchar> thresholdHistory;
    const int historySize = 5;
    
    thresholdHistory.push_back(currentThreshold);
    
    if (thresholdHistory.size() > historySize)
    {
        thresholdHistory.pop_front();
    }
    
    // Use median for stability
    std::vector<uchar> sortedHistory(thresholdHistory.begin(), thresholdHistory.end());
    std::sort(sortedHistory.begin(), sortedHistory.end());
    
    uchar medianThreshold = sortedHistory[sortedHistory.size() / 2];
    
    // Apply exponential moving average for smooth transitions
    static float smoothedThreshold = -1.0f;
    
    if (smoothedThreshold < 0)
    {
        smoothedThreshold = medianThreshold;
    }
    else
    {
        float alpha = 0.3f; // Smoothing factor
        smoothedThreshold = alpha * medianThreshold + (1.0f - alpha) * smoothedThreshold;
    }
    
    return static_cast<uchar>(smoothedThreshold);
}

// Add these member variables to AFurniLife class header:
// private:
//     bool bDebugMode = false;  // Enable debug logging for threshold selection
//     std::deque<uchar> thresholdHistory;  // For temporal smoothing

// Stub implementations for unused functions
void AFurniLife::UpdateTextureSafely()
{
    // Not used - direct update in ReadFrame
}

void AFurniLife::AssignTextureToImagePlate()
{
    // Already done in InitializeCamera
}

void AFurniLife::OnCameraFrameFromPixelBuffer(CVPixelBufferRef buffer)
{
    // Not currently used but kept for future native camera implementation
}
