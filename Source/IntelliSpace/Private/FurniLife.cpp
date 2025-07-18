#include "FurniLife.h"

#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "Engine/Texture2D.h"
#include "RenderUtils.h"
#include "RHICommandList.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Rendering/Texture2DResource.h"
#include "ImagePlateComponent.h"

//#if PLATFORM_IOS
//#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
//#endif

//#include "opencv2/opencv.hpp"
//#include <onnxruntime_cxx_api.h>

//namespace Ort { struct Session; }

AFurniLife::AFurniLife(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    UE_LOG(LogTemp, Warning, TEXT("FurniLife Constructor"));
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    rootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    ImagePlateRaw = CreateDefaultSubobject<UImagePlateComponent>(TEXT("ImagePlateRaw"));
    ImagePlatePost = CreateDefaultSubobject<UImagePlateComponent>(TEXT("ImagePlatePost"));
    Brightness = 0;
    Multiply = 1;
    CameraID = 0;
    VideoTrackID = 0;
    isStreamOpen = false;
    VideoSize = FVector2D(1920, 1080);
    RefreshRate = 30.0f;
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
    cap.open(CameraID);
    if (!cap.isOpened()) {
        UE_LOG(LogTemp, Error, TEXT("Failed to open camera"));
        return;
    }
    isStreamOpen = true;
    ColorData.SetNumUninitialized(VideoSize.X * VideoSize.Y);
    cvSize = cv::Size(VideoSize.X, VideoSize.Y);
    cvMat = cv::Mat(cvSize, CV_8UC4, ColorData.GetData());
    Camera_Texture2D = UTexture2D::CreateTransient(VideoSize.X, VideoSize.Y, PF_B8G8R8A8);
    VideoMask_Texture2D = UTexture2D::CreateTransient(VideoSize.X, VideoSize.Y, PF_G8);
#if WITH_EDITORONLY_DATA
    Camera_Texture2D->MipGenSettings = TMGS_NoMipmaps;
#endif
    Camera_Texture2D->SRGB = Camera_RenderTarget->SRGB;
    VideoMask_Texture2D->SRGB = false;
    Camera_Texture2D->UpdateResource();

    
    
    FString ModelPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("/U2NET1.onnx"));
    
#if PLATFORM_IOS
    OrtSession = new Ort::Session(OrtEnv, TCHAR_TO_UTF8(*ModelPath), SessionOptions);
    Ort::AllocatorWithDefaultOptions allocator;
    size_t inputCount = OrtSession->GetInputCount();
    for (size_t i = 0; i < inputCount; ++i) {
        Ort::AllocatedStringPtr input_name_ptr = OrtSession->GetInputNameAllocated(i, allocator);
        InputNames.push_back(input_name_ptr.get());
    }
    size_t outputCount = OrtSession->GetOutputCount();
    for (size_t i = 0; i < outputCount; ++i) {
        Ort::AllocatedStringPtr output_name_ptr = OrtSession->GetOutputNameAllocated(i, allocator);
        OutputNames.push_back(output_name_ptr.get());
    }
#endif
}

void AFurniLife::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    RefreshTimer += DeltaTime;
    if (isStreamOpen && RefreshTimer >= 1.0f / RefreshRate)
    {
        RefreshTimer = 0.0f;
        AsyncTask(ENamedThreads::GameThread, [this]() {
            if (ReadFrame()) {
                GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AFurniLife::OnNextVideoFrame);
            }
        });
    }
}

bool AFurniLife::ReadFrame()
{
    if (!cap.isOpened()) return false;
    cap.read(frame);
    if (frame.empty()) return false;
    cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA);
    
    ProcessInputForModel();
    RunModelInference();
    ApplySegmentationMask();
    
    for (int y = 0; y < frame.rows; ++y)
    {
        for (int x = 0; x < frame.cols; ++x)
        {
            cv::Vec4b pixel = frame.at<cv::Vec4b>(y, x);
            ColorData[y * frame.cols + x] = FColor(pixel[2], pixel[1], pixel[0], pixel[3]);
        }
    }
    static FUpdateTextureRegion2D Region(0, 0, 0, 0, VideoSize.X, VideoSize.Y);
    UpdateTextureRegions(Camera_Texture2D, 0, 1, &Region, VideoSize.X * sizeof(FColor), sizeof(FColor), (uint8*)ColorData.GetData(), false);
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
    const int Width = 320, Height = 320;
    std::vector<float> InputTensor(1 * 3 * Height * Width);
    int idx = 0;
    for (int c = 0; c < 3; ++c)
        for (int y = 0; y < Height; ++y)
            for (int x = 0; x < Width; ++x)
                InputTensor[idx++] = resized.at<cv::Vec3f>(y, x)[c];
    
#if PLATFORM_IOS
    Ort::MemoryInfo MemInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<int64_t> InputShape = {1, 3, Height, Width};
    Ort::Value InputTensorVal = Ort::Value::CreateTensor<float>(MemInfo, InputTensor.data(), InputTensor.size(), InputShape.data(), InputShape.size());
    auto OutputTensors = OrtSession->Run(Ort::RunOptions{nullptr}, InputNames.data(), &InputTensorVal, 1, OutputNames.data(), 1);
    float* OutData = OutputTensors[0].GetTensorMutableData<float>();
    OutputBuffer.assign(OutData, OutData + Height * Width);
#endif
    
}

void AFurniLife::ApplySegmentationMask()
{
    const int Width = 320, Height = 320;
    cv::Mat mask(Height, Width, CV_32FC1, OutputBuffer.data());
    mask.convertTo(alphaMask, CV_8UC1, 255.0);
    cv::resize(alphaMask, alphaMask, cv::Size(VideoSize.X, VideoSize.Y));
    if (frame.channels() == 3)
        cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA);
    for (int y = 0; y < frame.rows; ++y)
    {
        for (int x = 0; x < frame.cols; ++x)
        {
            uchar alpha = alphaMask.at<uchar>(y, x);
            frame.at<cv::Vec4b>(y, x)[3] = alpha < 64 ? 0 : alpha;
        }
    }
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
            uint8* Data;
            uint32 Pitch;
            uint32 Bpp;
            bool bFree;
        };
        FUpdateContext* Context = new FUpdateContext{Texture, MipIndex, Regions, SrcData, SrcPitch, SrcBpp, bFreeData};
        ENQUEUE_RENDER_COMMAND(UpdateTextureRegionsData)(
            [Context](FRHICommandListImmediate& RHICmdList)
            {
                FTexture2DResource* TexRes = static_cast<FTexture2DResource*>(Context->Tex->GetResource());
                RHIUpdateTexture2D(TexRes->GetTexture2DRHI(), Context->Mip, *Context->Region, Context->Pitch, Context->Data);
                if (Context->bFree) FMemory::Free(Context->Data);
                delete Context;
            });
    }
}

