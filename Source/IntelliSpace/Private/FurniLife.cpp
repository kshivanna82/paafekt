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
    
    // Enable segmentation
    bool bEnableSegmentation = true;
    
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
    
    // Calculate smart threshold
    uchar threshold = CalculateSmartThreshold(normalized);
    threshold = ApplyTemporalSmoothing(threshold);
    
    // Create binary mask
    cv::Mat binaryMask;
    cv::threshold(normalized, binaryMask, threshold, 255, cv::THRESH_BINARY);
    
    // IMPROVED HOLE FILLING WITH LARGEST COMPONENT DETECTION
    
    // Step 1: Morphological operations to clean up the mask
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7));
    cv::morphologyEx(binaryMask, binaryMask, cv::MORPH_CLOSE, kernel);
    
    // Small erosion then dilation to remove noise
    cv::Mat smallKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(binaryMask, binaryMask, cv::MORPH_OPEN, smallKernel);
    
    // Step 2: Find all contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binaryMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // Step 3: Create filled mask with largest component only
    cv::Mat filledMask = cv::Mat::zeros(binaryMask.size(), CV_8UC1);
    
    if (!contours.empty())
    {
        // Find the largest contour (assumed to be the person)
        size_t largestIdx = 0;
        double largestArea = 0;
        
        for (size_t i = 0; i < contours.size(); i++)
        {
            double area = cv::contourArea(contours[i]);
            if (area > largestArea)
            {
                largestArea = area;
                largestIdx = i;
            }
        }
        
        // Draw the largest contour filled
        cv::drawContours(filledMask, contours, largestIdx, cv::Scalar(255), cv::FILLED);
        
        // Optional: Include nearby contours that might be part of the person
        cv::Rect mainBBox = cv::boundingRect(contours[largestIdx]);
        
        // Expand bounding box slightly to catch disconnected parts
        int expansion = 20;
        mainBBox.x = std::max(0, mainBBox.x - expansion);
        mainBBox.y = std::max(0, mainBBox.y - expansion);
        mainBBox.width = std::min(filledMask.cols - mainBBox.x, mainBBox.width + 2 * expansion);
        mainBBox.height = std::min(filledMask.rows - mainBBox.y, mainBBox.height + 2 * expansion);
        
        for (size_t i = 0; i < contours.size(); i++)
        {
            if (i != largestIdx)
            {
                double area = cv::contourArea(contours[i]);
                // Only consider contours of reasonable size
                if (area > 50)
                {
                    cv::Rect bbox = cv::boundingRect(contours[i]);
                    // Check if this contour overlaps with expanded main bounding box
                    if ((mainBBox & bbox).area() > 0)
                    {
                        cv::drawContours(filledMask, contours, i, cv::Scalar(255), cv::FILLED);
                    }
                }
            }
        }
    }
    
    // Step 4: Smooth the edges for better visual quality
    cv::GaussianBlur(filledMask, filledMask, cv::Size(5, 5), 2);
    
    // Resize to video size
    cv::resize(filledMask, alphaMask, cv::Size(VideoSize.X, VideoSize.Y));
    
    // Ensure frame is BGRA
    if (frame.channels() == 3)
    {
        cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA);
    }
    
    // Apply the mask with smooth edge blending
    for (int y = 0; y < frame.rows; ++y)
    {
        for (int x = 0; x < frame.cols; ++x)
        {
            uchar alpha = alphaMask.at<uchar>(y, x);
            cv::Vec4b& px = frame.at<cv::Vec4b>(y, x);
            
            if (alpha < 30)
            {
                // Background - make green
                px[0] = 0;   // B
                px[1] = 255; // G
                px[2] = 0;   // R
                px[3] = 255; // A
            }
            else if (alpha < 225)
            {
                // Edge blending zone for smoother transitions
                float blend = (alpha - 30) / 195.0f;
                
                // Interpolate between green and original
                px[0] = static_cast<uchar>(px[0] * blend);
                px[1] = static_cast<uchar>(255 * (1 - blend) + px[1] * blend);
                px[2] = static_cast<uchar>(px[2] * blend);
                px[3] = 255;
            }
            else
            {
                // Foreground - keep original
                px[3] = 255;
            }
        }
    }
}

// Smart threshold calculation methods
uchar AFurniLife::CalculateOtsuThreshold(const cv::Mat& mask)
{
    cv::Mat binary;
    double otsuThreshold = cv::threshold(mask, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    float biasFactor = 0.8f;
    return static_cast<uchar>(otsuThreshold * biasFactor);
}

uchar AFurniLife::CalculatePercentileThreshold(const cv::Mat& mask)
{
    cv::Mat flat = mask.reshape(1, mask.total());
    cv::Mat sorted;
    cv::sort(flat, sorted, cv::SORT_ASCENDING);
    float percentile = 0.3f;
    int index = static_cast<int>(sorted.total() * percentile);
    return sorted.at<uchar>(index);
}

uchar AFurniLife::CalculateHistogramThreshold(const cv::Mat& mask)
{
    int histSize = 256;
    float range[] = {0, 256};
    const float* histRange = {range};
    cv::Mat hist;
    cv::calcHist(&mask, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange);
    
    cv::GaussianBlur(hist, hist, cv::Size(1, 5), 0);
    
    int peak1 = 0, peak2 = 255;
    float maxVal1 = 0, maxVal2 = 0;
    
    for (int i = 0; i < 128; i++)
    {
        float val = hist.at<float>(i);
        if (val > maxVal1)
        {
            maxVal1 = val;
            peak1 = i;
        }
    }
    
    for (int i = 128; i < 256; i++)
    {
        float val = hist.at<float>(i);
        if (val > maxVal2)
        {
            maxVal2 = val;
            peak2 = i;
        }
    }
    
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

uchar AFurniLife::CalculateAdaptiveThreshold(const cv::Mat& mask)
{
    cv::Scalar mean, stddev;
    cv::meanStdDev(mask, mean, stddev);
    float k = 0.5f;
    float threshold = mean[0] - k * stddev[0];
    threshold = std::max(10.0f, std::min(245.0f, threshold));
    return static_cast<uchar>(threshold);
}

uchar AFurniLife::CalculateSmartThreshold(const cv::Mat& mask)
{
    cv::Scalar mean, stddev;
    cv::meanStdDev(mask, mean, stddev);
    
    float coefficientOfVariation = stddev[0] / (mean[0] + 1e-6);
    
    uchar threshold;
    
    if (coefficientOfVariation > 0.8f)
    {
        threshold = CalculateOtsuThreshold(mask);
    }
    else if (coefficientOfVariation < 0.3f)
    {
        threshold = CalculatePercentileThreshold(mask);
    }
    else
    {
        threshold = CalculateHistogramThreshold(mask);
    }
    
    float confidence = CalculateSegmentationConfidence(mask, threshold);
    
    if (confidence < 0.6f)
    {
        uchar adaptiveThreshold = CalculateAdaptiveThreshold(mask);
        threshold = static_cast<uchar>(0.7f * threshold + 0.3f * adaptiveThreshold);
    }
    
    return threshold;
}

float AFurniLife::CalculateSegmentationConfidence(const cv::Mat& mask, uchar threshold)
{
    int foregroundCount = 0;
    int backgroundCount = 0;
    int uncertainCount = 0;
    int uncertaintyBand = 20;
    
    for (int y = 0; y < mask.rows; ++y)
    {
        for (int x = 0; x < mask.cols; ++x)
        {
            uchar val = mask.at<uchar>(y, x);
            
            if (val > threshold + uncertaintyBand)
                foregroundCount++;
            else if (val < threshold - uncertaintyBand)
                backgroundCount++;
            else
                uncertainCount++;
        }
    }
    
    int totalPixels = mask.rows * mask.cols;
    float uncertainRatio = static_cast<float>(uncertainCount) / totalPixels;
    float confidence = 1.0f - uncertainRatio;
    
    float foregroundRatio = static_cast<float>(foregroundCount) / totalPixels;
    if (foregroundRatio < 0.05f || foregroundRatio > 0.95f)
        confidence *= 0.5f;
    
    return confidence;
}

uchar AFurniLife::ApplyTemporalSmoothing(uchar currentThreshold)
{
    static std::deque<uchar> thresholdHistory;
    const int historySize = 5;
    
    thresholdHistory.push_back(currentThreshold);
    
    if (thresholdHistory.size() > historySize)
    {
        thresholdHistory.pop_front();
    }
    
    std::vector<uchar> sortedHistory(thresholdHistory.begin(), thresholdHistory.end());
    std::sort(sortedHistory.begin(), sortedHistory.end());
    
    uchar medianThreshold = sortedHistory[sortedHistory.size() / 2];
    
    static float smoothedThreshold = -1.0f;
    
    if (smoothedThreshold < 0)
    {
        smoothedThreshold = medianThreshold;
    }
    else
    {
        float alpha = 0.3f;
        smoothedThreshold = alpha * medianThreshold + (1.0f - alpha) * smoothedThreshold;
    }
    
    return static_cast<uchar>(smoothedThreshold);
}

// Stub implementations
void AFurniLife::UpdateTextureSafely()
{
}

void AFurniLife::AssignTextureToImagePlate()
{
}

void AFurniLife::OnCameraFrameFromPixelBuffer(CVPixelBufferRef buffer)
{
}
