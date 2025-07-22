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
#include <mutex>
#endif



AFurniLife::AFurniLife(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
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
    
    UE_LOG(LogTemp, Warning, TEXT("FurniLife::BeginPlay (before Super) this=%p"), this);
    UE_LOG(LogTemp, Warning, TEXT("rootComp=%p, ImagePlateRaw=%p, ImagePlatePost=%p"), rootComp, ImagePlateRaw, ImagePlatePost);

    Super::BeginPlay();
    UE_LOG(LogTemp, Warning, TEXT("Returned from Super::BeginPlay()"));


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

#if PLATFORM_IOS
    FString OnnxPath = FString([[NSBundle mainBundle] pathForResource:@"U2NET1" ofType:@"onnx"]);
    TArray<uint8> FileData;
    std::ifstream file(TCHAR_TO_UTF8(*OnnxPath), std::ios::binary | std::ios::ate);
    if (file)
    {
        std::ifstream::pos_type size = file.tellg();
        FileData.SetNumUninitialized(size);
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(FileData.GetData()), size);
        file.close();
    }

    Ort::SessionOptions SessionOptions;
    SessionOptions.SetIntraOpNumThreads(1);
    SessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
    OrtSession = new Ort::Session(OrtEnv, FileData.GetData(), FileData.Num(), SessionOptions);

    OrtAllocator* RawAllocator = nullptr;
    Ort::ThrowOnError(OrtGetApiBase()->GetApi(ORT_API_VERSION)->GetAllocatorWithDefaultOptions(&RawAllocator));

    char* inputName = nullptr;
    Ort::ThrowOnError(OrtGetApiBase()->GetApi(ORT_API_VERSION)->SessionGetInputName(*OrtSession, 0, RawAllocator, &inputName));
    InputNameStrs.emplace_back(inputName);
    InputNames.Add(InputNameStrs.back().c_str());
    Ort::GetApi().AllocatorFree(RawAllocator, inputName);

    char* outputName = nullptr;
    Ort::ThrowOnError(OrtGetApiBase()->GetApi(ORT_API_VERSION)->SessionGetOutputName(*OrtSession, 0, RawAllocator, &outputName));
    OutputNameStrs.emplace_back(outputName);
    OutputNames.Add(OutputNameStrs.back().c_str());
    Ort::GetApi().AllocatorFree(RawAllocator, outputName);
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
    if (!Camera_Texture2D || !cap.isOpened())
        return false;
    cap.read(frame);
    if (frame.empty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Camera frame is empty"));
        return false;
    }
    
    ProcessInputForModel();

    RunModelInference();
    ApplySegmentationMask();
    
    // Convert to BGRA (for Unreal)
    cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA);

    
//    cv::Mat flipped;
//    cv::flip(frame, flipped, 0); // Flip vertically for Unreal
    int Width = VideoSize.X;
    int Height = VideoSize.Y;
    for (int y = 0; y < Height; ++y)
    {
        for (int x = 0; x < Width; ++x)
        {
//            cv::Vec4b& srcPixel = frame.at<cv::Vec4b>(y, x);
            cv::Vec4b& srcPixel = frame.at<cv::Vec4b>(y, x);
            int index = y * Width + x;
            ColorData[index] = FColor(srcPixel[2], srcPixel[1], srcPixel[0], srcPixel[3]);
        }
    }

    static FUpdateTextureRegion2D Region(0, 0, 0, 0, VideoSize.X, VideoSize.Y);
    UpdateTextureRegions(
        Camera_Texture2D,
        0,
        1,
        &Region,
        VideoSize.X * sizeof(FColor),
        sizeof(FColor),
        reinterpret_cast<uint8*>(ColorData.GetData()),
        false
    );
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
    std::vector<float> InputTensor(1 * 3 * Height * Width);
    int idx = 0;
    for (int c = 0; c < 3; ++c)
        for (int y = 0; y < Height; ++y)
            for (int x = 0; x < Width; ++x)
                InputTensor[idx++] = resized.at<cv::Vec3f>(y, x)[c];

    Ort::MemoryInfo MemInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<int64_t> InputShape = {1, 3, Height, Width};
    Ort::Value InputTensorVal = Ort::Value::CreateTensor<float>(MemInfo, InputTensor.data(), InputTensor.size(), InputShape.data(), InputShape.size());

    auto OutputTensors = OrtSession->Run(
        Ort::RunOptions{nullptr},
        InputNames.GetData(), &InputTensorVal, 1,
        OutputNames.GetData(), 1);

    float* OutData = OutputTensors[0].GetTensorMutableData<float>();
    OutputBuffer.assign(OutData, OutData + Height * Width);
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
    cv::Mat mask(Height, Width, CV_32FC1, OutputBuffer.data());
#endif
    mask.convertTo(alphaMask, CV_8UC1, 255.0);
    cv::resize(alphaMask, alphaMask, cv::Size(VideoSize.X, VideoSize.Y));  // Resize back to full resolution
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
            px[3] = (alpha < 64) ? 0 : alpha;
        }
    }
    double minVal, maxVal;
    cv::minMaxLoc(alphaMask, &minVal, &maxVal);
    UE_LOG(LogTemp, Warning, TEXT("Alpha mask range: min = %f, max = %f"), minVal, maxVal);

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
