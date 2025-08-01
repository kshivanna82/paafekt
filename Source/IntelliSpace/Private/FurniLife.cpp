#include "FurniLife.h"
#include "Misc/Paths.h"
//#include "HAL/PlatformFilemanager.h"
#include "Engine/Texture2D.h"
#include "RenderUtils.h"
#include "RHICommandList.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Rendering/Texture2DResource.h"
#include "ImagePlateComponent.h"
#if PLATFORM_IOS
#include <Foundation/Foundation.h>
#include <fstream>
//#include <mutex>
#import <UIKit/UIKit.h>
#import "CoreMLModelBridge.h"
//FCoreMLModelBridge* CoreMLBridge = nullptr;
#endif
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"  // For TActorIterator
#include "CineCameraActor.h"
//#include "CineCameraComponent.h"
#include "Kismet/GameplayStatics.h"

AFurniLife* AFurniLife::CurrentInstance = nullptr;
ACineCameraActor* CineCameraActor = nullptr;


AFurniLife::AFurniLife(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    rootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
//    ImagePlateRaw = CreateDefaultSubobject<UImagePlateComponent>(TEXT("ImagePlateRaw"));
    ImagePlatePost = CreateDefaultSubobject<UImagePlateComponent>(TEXT("ImagePlatePost"));
//    ImagePlatePost->SetupAttachment(rootComp);
//    ImagePlatePost->SetRelativeLocation(FVector(-245.0f, 395.0f, 226.0f));  // Tune this
//    ImagePlatePost->SetRelativeRotation(FRotator(-4.0f, -11.0f, -51.0f));
//    ImagePlatePost->SetRelativeScale3D(FVector(1.0f, 200.0f, 200.0f));
    
    // Get PlayerStart transform
//    ImagePlateComponent->AttachToComponent(CineCamera->GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
//    ImagePlateComponent->SetRelativeLocation(FVector(100.f, 0.f, 0.f));  // Adjust distance
//    ImagePlateComponent->SetRelativeScale3D(FVector(1.f));              // Reset scale
    
    
//    ImagePlatePost->AttachToComponent(CineCamera->GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
    //KKKKKKKKKKKK
//    ImagePlatePost->SetRelativeLocation(FVector(100.f, 0.f, 0.f));  // Adjust distance
//    ImagePlatePost->SetRelativeScale3D(FVector(1.f));              // Reset scale

    


    Brightness = 0;
    Multiply = 1;
    CameraID = 0;
    VideoTrackID = 0;
    isStreamOpen = false;
    VideoSize = FVector2D(1920, 1080);
    RefreshRate = 30.0f;
    #if PLATFORM_IOS
//    CoreMLBridge = nullptr;
    #endif
}

static void UpdateTextureRegions(UTexture2D* Texture, int32 MipIndex, uint32 NumRegions, FUpdateTextureRegion2D* Regions,
                                 uint32 SrcPitch, uint32 SrcBpp, uint8* SrcData, bool bFreeData)
{
    if (Texture && Texture->GetResource())
    {
        struct FUpdateContext
        {
            UTexture2D* Tex;
            int32 Mip;
            FUpdateTextureRegion2D* Region;
            const uint8* Data;
            uint32 Pitch;
            uint32 Bpp;
            bool bFree;
        };

        FUpdateContext* Context = new FUpdateContext{Texture, MipIndex, Regions, SrcData, SrcPitch, SrcBpp, bFreeData};

        ENQUEUE_RENDER_COMMAND(UpdateTextureRegionsData)(
            [Context](FRHICommandListImmediate& RHICmdList)
            {
                FTexture2DResource* TexRes = static_cast<FTexture2DResource*>(Context->Tex->GetResource());
#if PLATFORM_IOS
                uint32 DestStride = 0;
                void* TextureMemory = RHICmdList.LockTexture2D(
                    TexRes->GetTexture2DRHI(),
                    Context->Mip,
                    RLM_WriteOnly,
                    DestStride,
                    false);

                const uint32 WidthInBytes = Context->Region->Width * Context->Bpp;
                for (uint32 y = 0; y < Context->Region->Height; ++y)
                {
                    uint8* DestRow = static_cast<uint8*>(TextureMemory) + y * DestStride;
                    const uint8* SrcRow = Context->Data + y * Context->Pitch;
                    FMemory::Memcpy(DestRow, SrcRow, WidthInBytes);
//                    for (int32 Row = 0; Row < Region.Height; ++Row)
//                    {
//                        FMemory::Memcpy(
//                            reinterpret_cast<uint8*>(DestData) + Row * DestStride,
//                            SrcData + Row * RowPitch,
//                            RowPitch
//                        );
//                    }

                }

                RHICmdList.UnlockTexture2D(TexRes->GetTexture2DRHI(), Context->Mip, false);
                RHIFlushResources();
#endif
#if PLATFORM_MAC
//                FTexture2DResource* TexRes = (FTexture2DResource*)Texture->Resource;
                if (!TexRes)
                {
                    UE_LOG(LogTemp, Error, TEXT("❌ Texture resource is null!"));
                    return;
                }
                UE_LOG(LogTemp, Error, TEXT("Trying RHIUpdateTexture2D on Mac with size: %d x %d, pitch: %d, bpp: %d"),
                       Context->Region->Width, Context->Region->Height, Context->Pitch, Context->Bpp);

                RHIUpdateTexture2D(
                    TexRes->GetTexture2DRHI(),
                    Context->Mip,
                    *Context->Region,
                    Context->Pitch,
                    Context->Data);
#endif
                if (Context->bFree)
                {
                    FMemory::Free(const_cast<uint8*>(Context->Data));

                }
                delete Context;
            });
    }
}



static void UpdateTextureRegions(
    UTexture2D* Texture,
    int32 MipIndex,
    uint32 NumRegions,
    FUpdateTextureRegion2D* Regions,
    uint32 SrcPitch,
    uint32 SrcBpp,
    uint8* SrcData,
    bool bFreeData = false
);

void AFurniLife::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Warning, TEXT("Returned from Super::BeginPlay()"));

    AFurniLife::CurrentInstance = this;
    cap.open(CameraID);
    if (!cap.isOpened()) {
        UE_LOG(LogTemp, Error, TEXT("Failed to open camera"));
        return;
    }
    cap.set(cv::CAP_PROP_BUFFERSIZE, 1);
    isStreamOpen = true;

    ColorData.SetNumUninitialized(VideoSize.X * VideoSize.Y);
    cvSize = cv::Size(VideoSize.X, VideoSize.Y);
    cvMat = cv::Mat(cvSize, CV_8UC4, ColorData.GetData());

    Camera_Texture2D = UTexture2D::CreateTransient(VideoSize.X, VideoSize.Y, PF_B8G8R8A8);
    VideoMask_Texture2D = UTexture2D::CreateTransient(VideoSize.X, VideoSize.Y, PF_G8);
#if WITH_EDITORONLY_DATA
    Camera_Texture2D->MipGenSettings = TMGS_NoMipmaps;
#endif
//    Camera_Texture2D->LODGroup = TEXTUREGROUP_UI;
//    Camera_Texture2D->CompressionSettings = TC_Default;
    Camera_Texture2D->SRGB = Camera_RenderTarget->SRGB;
    Camera_Texture2D->AddToRoot();
//    Camera_Texture2D->NeverStream = true;
//    Camera_Texture2D->SRGB = false;
    VideoMask_Texture2D->SRGB = false;
    Camera_Texture2D->UpdateResource();
    
//     Find the first CineCamera in the world
    for (TActorIterator<ACineCameraActor> It(GetWorld()); It; ++It)
    {
        CineCameraActor = *It;
        break; // Take the first one found
    }
    
    APlayerStart* PlayerStart = nullptr;
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        PlayerStart = *It;
        break;
    }

    if (PlayerStart)
    {
        FVector ForwardOffset = PlayerStart->GetActorForwardVector() * 25.0f;  // 25 units in front
        FVector PlacementLocation = PlayerStart->GetActorLocation() + ForwardOffset + FVector(0, 0, 100);  // Add height if needed
        FRotator PlacementRotation = PlayerStart->GetActorRotation();  // Match player's facing direction

        if (!ImagePlatePost)
        {
            UE_LOG(LogTemp, Error, TEXT("❌ ImagePlatePost is null!"));
            return;
        }

        ImagePlatePost->SetWorldLocation(PlacementLocation);
        ImagePlatePost->SetWorldRotation(PlacementRotation);
    }

#if PLATFORM_IOS
//    StartCameraStreaming();
    //Kishore
    // CoreML Model Path
    NSString* ModelPath = [[NSBundle mainBundle] pathForResource:@"u2net" ofType:@"mlmodelc"];
    if (!ModelPath) {
        UE_LOG(LogTemp, Error, TEXT("❌ Could not find u2net.mlmodelc in bundle."));
        return;
    }

//    CoreMLBridge = new FCoreMLModelBridge();
    if (!GetSharedBridge()->LoadModel([ModelPath UTF8String])) {
//    if (!CoreMLBridge->LoadModel([ModelPath UTF8String])) {
        UE_LOG(LogTemp, Error, TEXT("❌ Failed to load CoreML model."));
        return;
    }
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
    }
#endif
}

static double LastProcessTime = 0;
static double LastCameraCheckTime = 0;


void AFurniLife::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    double CurrentTime = GetWorld()->TimeSeconds;

    // 🕒 1. Reopen camera every ~3 seconds
    if (CurrentTime - LastCameraCheckTime >= 3.0)
    {
        LastCameraCheckTime = CurrentTime;
        cap.release();
        cap.open(CameraID);
        UE_LOG(LogTemp, Warning, TEXT("🔁 Periodic camera reopen triggered."));
    }

    // 🖼️ 2. Throttle frame processing using RefreshRate
    if (isStreamOpen && (CurrentTime - LastProcessTime) >= (1.0 / RefreshRate))
    {
        LastProcessTime = CurrentTime;

        if (!cap.isOpened())
        {
            cap.release();
            cap.open(CameraID);
            UE_LOG(LogTemp, Warning, TEXT("📷 OpenCV VideoCapture was closed. Reopening."));
        }

        AsyncTask(ENamedThreads::GameThread, [this]() {
            if (ReadFrame()) {
                GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AFurniLife::OnNextVideoFrame);
            }
        });
    }
}

void AFurniLife::OnCameraFrameFromPixelBuffer(CVPixelBufferRef buffer)
{
    // Convert to cv::Mat
    CVPixelBufferLockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
    int width = CVPixelBufferGetWidth(buffer);
    int height = CVPixelBufferGetHeight(buffer);
    uint8_t* base = (uint8_t*)CVPixelBufferGetBaseAddress(buffer);
    size_t stride = CVPixelBufferGetBytesPerRow(buffer);

    cv::Mat mat(height, width, CV_8UC4, base, stride);
    frame = mat.clone();
    CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);

    ProcessInputForModel();
//    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        
        RunModelInference();  // Already calls bridge->RunModel(PixelBuffer)
        ApplySegmentationMask();
        // Convert to BGRA (for Unreal)
        cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA);
        
        
        // Log top-left and bottom-right pixel (after alpha applied)
        cv::Vec4b topLeft = frame.at<cv::Vec4b>(0, 0);
        cv::Vec4b bottomRight = frame.at<cv::Vec4b>(frame.rows - 1, frame.cols - 1);
        
        UE_LOG(LogTemp, Warning, TEXT("[Pixel Debug] Top-Lefttttttt (BGRA): %d %d %d %d"),
               topLeft[0], topLeft[1], topLeft[2], topLeft[3]);
        UE_LOG(LogTemp, Warning, TEXT("[Pixel Debug] Bottom-Rightttttttt (BGRA): %d %d %d %d"),
               bottomRight[0], bottomRight[1], bottomRight[2], bottomRight[3]);
        
        
        
        
        // Print center pixel for debug
        int cx = frame.cols / 2;
        int cy = frame.rows / 2;
        cv::Vec4b center = frame.at<cv::Vec4b>(cy, cx);
        UE_LOG(LogTemp, Warning, TEXT("[Pixel Debug] Center (BGRA): %d %d %d %d"), center[0], center[1], center[2], center[3]);
        
        
        
        int Width = frame.cols;
        int Height = frame.rows;
        VideoSize = FVector2D(Width, Height);  // Update VideoSize safely
        
        ColorData.SetNumUninitialized(Width * Height);
        
        for (int y = 0; y < Height; ++y)
        {
            for (int x = 0; x < Width; ++x)
            {
                cv::Vec4b& srcPixel = frame.at<cv::Vec4b>(y, x);
                int index = y * Width + x;
                //            ColorData.SetNum(Width * Height, EAllowShrinking::No);
                
                ColorData[index] = FColor(srcPixel[2], srcPixel[1], srcPixel[0], srcPixel[3]);
                //            ColorData[index] = FColor(255, 0, 0, 255);  // Solid opaque red
            }
        }
        int32 SrcPitch = static_cast<int32>(VideoSize.X * sizeof(FColor));
        UE_LOG(LogTemp, Warning, TEXT("SrcPitchjkfjgkfjgkgjhkjhkj = %d"), SrcPitch);
        
        UE_LOG(LogTemp, Warning, TEXT("frame step = %llu, cols = %d, elemSize = %d"),
               static_cast<uint64>(frame.step), frame.cols, frame.elemSize());
        
        UE_LOG(LogTemp, Warning, TEXT("frame size = %d x %d, isContinuous = %s, type = %d"),
               frame.cols, frame.rows,
               frame.isContinuous() ? TEXT("true") : TEXT("false"),
               static_cast<int>(frame.type()));
        
        
        //    if (!frame.isContinuous() || frame.type() != CV_8UC4)
        //    {
        //        cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA);
        //        frame = frame.clone();
        //    }
        
        
        UE_LOG(LogTemp, Warning, TEXT("❗ Frame size = %d x %d, elemSize = %zu, step = %zu, isContinuous = %s"),
               frame.cols, frame.rows, frame.elemSize(), frame.step[0], frame.isContinuous() ? TEXT("true") : TEXT("false"));
        
        UE_LOG(LogTemp, Warning, TEXT("🧵 About to update texture with BGRA frame 111111..."));
        
        TArray<FColor> LocalCopy = ColorData;
        FVector2D LocalSize = VideoSize;
        
//        cv::rotate(frame, frame, cv::ROTATE_90_COUNTERCLOCKWISE); // or ROTATE_90_COUNTERCLOCKWISE

        
        UE_LOG(LogTemp, Warning, TEXT("📍 GameThread check: %d"), IsInGameThread());
        
//        AsyncTask(ENamedThreads::GameThread, [=]() {
            if (!Camera_Texture2D) return;
            static FUpdateTextureRegion2D Region(0, 0, 0, 0, VideoSize.X, VideoSize.Y);
            UpdateTextureRegions(
                                 Camera_Texture2D,
                                 0,
                                 1,
                                 &Region,
                                 LocalSize.X * sizeof(FColor),
                                 //                         SrcPitch,
                                 sizeof(FColor),
                                 reinterpret_cast<uint8*>(ColorData.GetData()),
                                 false
                                 );
//        });
//    });
    return;
}

bool AFurniLife::ReadFrame()
{
    if (!Camera_Texture2D || !cap.isOpened())
        return false;
    cap.read(frame);
    
    if (frame.empty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Camera frame is empty"));
        return false;
    }
#if PLATFORM_IOS
    cv::rotate(frame, frame, cv::ROTATE_90_COUNTERCLOCKWISE);  // or try COUNTERCLOCKWISE if incorrect

     //kishoreeeee
    VideoSize = FVector2D(frame.cols, frame.rows);
#endif
    
    
    ProcessInputForModel();

    RunModelInference();
    ApplySegmentationMask();
    
    // Convert to BGRA (for Unreal)
    cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA);

    
    // Log top-left and bottom-right pixel (after alpha applied)
    cv::Vec4b topLeft = frame.at<cv::Vec4b>(0, 0);
    cv::Vec4b bottomRight = frame.at<cv::Vec4b>(frame.rows - 1, frame.cols - 1);

    UE_LOG(LogTemp, Warning, TEXT("[Pixel Debug] Top-Lefttttttt (BGRA): %d %d %d %d"),
           topLeft[0], topLeft[1], topLeft[2], topLeft[3]);
    UE_LOG(LogTemp, Warning, TEXT("[Pixel Debug] Bottom-Rightttttttt (BGRA): %d %d %d %d"),
           bottomRight[0], bottomRight[1], bottomRight[2], bottomRight[3]);

    
    
    
    // Print center pixel for debug
        int cx = frame.cols / 2;
        int cy = frame.rows / 2;
        cv::Vec4b center = frame.at<cv::Vec4b>(cy, cx);
        UE_LOG(LogTemp, Warning, TEXT("[Pixel Debug] Center (BGRA): %d %d %d %d"), center[0], center[1], center[2], center[3]);

    
    cv::resize(frame, frame, cv::Size(VideoSize.X, VideoSize.Y));
    int Width = VideoSize.X;
    int Height = VideoSize.Y;
    for (int y = 0; y < Height; ++y)
    {
        for (int x = 0; x < Width; ++x)
        {
            cv::Vec4b& srcPixel = frame.at<cv::Vec4b>(y, x);
            int index = y * Width + x;
            ColorData[index] = FColor(srcPixel[2], srcPixel[1], srcPixel[0], srcPixel[3]);
//            ColorData[index] = FColor(255, 0, 0, 255);  // Solid opaque red
        }
    }
    int32 SrcPitch = static_cast<int32>(VideoSize.X * sizeof(FColor));
    UE_LOG(LogTemp, Warning, TEXT("SrcPitchjkfjgkfjgkgjhkjhkj = %d"), SrcPitch);

    UE_LOG(LogTemp, Warning, TEXT("frame step = %llu, cols = %d, elemSize = %d"),
           static_cast<uint64>(frame.step), frame.cols, frame.elemSize());

    UE_LOG(LogTemp, Warning, TEXT("frame size = %d x %d, isContinuous = %s, type = %d"),
           frame.cols, frame.rows,
           frame.isContinuous() ? TEXT("true") : TEXT("false"),
           static_cast<int>(frame.type()));

    
//    if (!frame.isContinuous() || frame.type() != CV_8UC4)
//    {
//        cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA);
//        frame = frame.clone();
//    }


    UE_LOG(LogTemp, Warning, TEXT("❗ Frame size = %d x %d, elemSize = %zu, step = %zu, isContinuous = %s"),
           frame.cols, frame.rows, frame.elemSize(), frame.step[0], frame.isContinuous() ? TEXT("true") : TEXT("false"));

    UE_LOG(LogTemp, Warning, TEXT("🧵 About to update texture with BGRA frame 22222..."));

    
//kishhh
    static FUpdateTextureRegion2D Region(0, 0, 0, 0, VideoSize.X, VideoSize.Y);
    //THIS COULD BE A FIX FOR SOMETHING
//    static FUpdateTextureRegion2D Region(0, 0, 0, 0, VideoSize.Y, VideoSize.X);
    UpdateTextureRegions(
        Camera_Texture2D,
        0,
        1,
        &Region,
        VideoSize.X * sizeof(FColor),
//                         SrcPitch,
        sizeof(FColor),
        reinterpret_cast<uint8*>(ColorData.GetData()),
        false
    );
    return false;
}

void AFurniLife::ProcessInputForModel()
{
    cv::resize(frame, resized, cv::Size(320, 320));
    cv::cvtColor(resized, resized, cv::COLOR_BGR2RGB);
    resized.convertTo(resized, CV_32FC3, 1.0 / 255.0);

}

//void AFurniLife::BeginDestroy()
//{
//#if PLATFORM_IOS
////    if (CoreMLBridge)
////    {
////        delete CoreMLBridge;
////        CoreMLBridge = nullptr;
////    }
//#endif
//    AFurniLife::CurrentInstance = nullptr;
//    Super::BeginDestroy();
//}


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
            // CoreML expects BGRA in CVPixelBuffer
            dstRow[x * 4 + 0] = px[0]; // B
            dstRow[x * 4 + 1] = px[1]; // G
            dstRow[x * 4 + 2] = px[2]; // R
            dstRow[x * 4 + 3] = px[3]; // A
        }
    }

    CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);

    std::vector<float> OutputData;
    if (!GetSharedBridge() || !GetSharedBridge()->RunModel(pixelBuffer, OutputData)) {
//    if (!CoreMLBridge || !CoreMLBridge->RunModel(pixelBuffer, OutputData)) {
        UE_LOG(LogTemp, Error, TEXT("❌ CoreML inference failed."));
        CVPixelBufferRelease(pixelBuffer);
        return;
    }

    OutputBuffer = std::move(OutputData);
    CVPixelBufferRelease(pixelBuffer);

    UE_LOG(LogTemp, Warning, TEXT("✅ CoreML inference success. Output size: %d"), (int)OutputBuffer.size());

    float maxOut = -FLT_MAX, minOut = FLT_MAX;
    for (float v : OutputBuffer) {
        if (v < minOut) minOut = v;
        if (v > maxOut) maxOut = v;
    }
    UE_LOG(LogTemp, Warning, TEXT("[Output Debug] CoreML raw output range: min = %f, max = %f"), minOut, maxOut);

#endif
#if PLATFORM_MAC
    const int Width = 320;
       const int Height = 320;
       if (!CpuModelInstance)
       {
           UE_LOG(LogTemp, Error, TEXT("[Inference] CpuModelInstance is null."));
           return;
       }
       TArray<float> InputTensor;
       InputTensor.SetNum(1 * 3 * Width * Height);
       int idx = 0;
       for (int c = 0; c < 3; ++c)
           for (int y = 0; y < Height; ++y)
               for (int x = 0; x < Width; ++x)
                   InputTensor[idx++] = resized.at<cv::Vec3f>(y, x)[c];
       TArray<uint32> InputDims = {1, 3, Height, Width};
       UE::NNE::FTensorShape InputShape = UE::NNE::FTensorShape::Make(InputDims);
       CpuModelInstance->SetInputTensorShapes({InputShape});
       UE::NNE::FTensorBindingCPU InputBinding;
       InputBinding.Data = InputTensor.GetData();
       InputBinding.SizeInBytes = InputTensor.Num() * sizeof(float);
       OutputTensors.Empty();
       OutputBindings.Empty();
       for (int i = 0; i < 7; ++i)
       {
           TArray<float> OutputTensor;
           OutputTensor.SetNum(Height * Width);  // One channel
           UE::NNE::FTensorBindingCPU Binding;
           Binding.Data = OutputTensor.GetData();
           Binding.SizeInBytes = OutputTensor.Num() * sizeof(float);
           OutputTensors.Add(MoveTemp(OutputTensor));
           OutputBindings.Add(Binding);
       }
       auto Status = CpuModelInstance->RunSync({InputBinding}, OutputBindings);
       if (Status != UE::NNE::EResultStatus::Ok) {
           UE_LOG(LogTemp, Error, TEXT("Inference failed."));
       }
       else {
           UE_LOG(LogTemp, Log, TEXT("Inference succeeded."));
       }

#endif
}

void AFurniLife::ApplySegmentationMask()
{

    const int Width = 320;
    const int Height = 320;
    #if PLATFORM_MAC
    const float* MaskOutput = OutputTensors.Last().GetData();
    cv::Mat mask(Height, Width, CV_32FC1, (void*)MaskOutput);
    #endif
    #if PLATFORM_IOS
    if (OutputBuffer.empty()) {
        UE_LOG(LogTemp, Error, TEXT("❌ OutputBuffer is empty. Skipping mask processing."));
        return;
    }
    cv::Mat mask(Height, Width, CV_32FC1, OutputBuffer.data());
    #endif

//#if PLATFORM_MAC
    // ADD: Clamp to [0.0, 1.0]
    cv::Mat clamped;
    cv::threshold(mask, clamped, 1.0f, 1.0f, cv::THRESH_TRUNC);

    // Convert to 8-bit
    clamped.convertTo(alphaMask, CV_8UC1, 255.0);
//#endif
//    
//#if PLATFORM_IOS
//    cv::Mat normalized;
//    cv::normalize(mask, normalized, 0.0f, 1.0f, cv::NORM_MINMAX);  // Force into [0, 1]
//    normalized.convertTo(alphaMask, CV_8UC1, 255.0);
//#endif
    

    // Resize
    cv::resize(alphaMask, alphaMask, cv::Size(VideoSize.X, VideoSize.Y));
    
    if (frame.channels() == 3)
    {
        UE_LOG(LogTemp, Warning, TEXT("in if of frameeeeeeeeeeeee.channels() : %d"), frame.channels());
        cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA);
    }
    int cx = alphaMask.cols / 2;
    int cy = alphaMask.rows / 2;
//    uchar centerAlpha = alphaMask.at<uchar>(cy, cx);
//    alphaMask.at<uchar>(cy, cx) = 255;
//    UE_LOG(LogTemp, Warning, TEXT("[Alpha Debug] Centereeeerrrrrrrrrrr alpha: %d"), centerAlpha);
    
    double minVal, maxVal;
    cv::minMaxLoc(alphaMask, &minVal, &maxVal);
    UE_LOG(LogTemp, Warning, TEXT("Alpha mask rangerrtttttttttttt: min = %f, max = %f"), minVal, maxVal);
    for (int y = 0; y < frame.rows; ++y)
    {
        for (int x = 0; x < frame.cols; ++x)
        {
            uchar alpha = alphaMask.at<uchar>(y, x);
            cv::Vec4b& px = frame.at<cv::Vec4b>(y, x);
//            px[3] = (alpha < 64) ? 0 : alpha;
//            px[3] = alpha;
//            px[3] = 255;
//            if (alpha < 64)  // Background
            int Threshold = FMath::Clamp((int)(maxVal * 0.5), 10, 64);

//            UE_LOG(LogTemp, Warning, TEXT("in if of Thre-sholdDDDDDDDDDD : %d"), Threshold);
            if (alpha < 32)
            {
                px[0] = 0;     // B
                px[1] = 255;   // G
                px[2] = 0;     // R
                px[3] = 255;   // Full alpha (opaque)
            }
            else  // Foreground
            {
                px[3] = 255;  // Ensure fully opaque
            }
        }
    }
    

}



