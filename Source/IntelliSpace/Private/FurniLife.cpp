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
    Camera_Texture2D->LODGroup = TEXTUREGROUP_UI;
    Camera_Texture2D->CompressionSettings = TC_Default;
    Camera_Texture2D->SRGB = Camera_RenderTarget->SRGB;
//    Camera_Texture2D->NeverStream = true;
//    Camera_Texture2D->SRGB = false;
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
    
//    OrtAllocator* RawAllocator = nullptr;
//    // INPUT name
//    char* inputName = nullptr;
//    Ort::ThrowOnError(OrtGetApiBase()->GetApi(ORT_API_VERSION)->SessionGetInputName(*OrtSession, 0, RawAllocator, &inputName));
//    InputNameStrs.emplace_back(inputName);
//    InputNames.Add(InputNameStrs.back().c_str());
//    Ort::GetApi().AllocatorFree(RawAllocator, inputName);

    OrtAllocator* RawAllocator = nullptr;
    Ort::ThrowOnError(OrtGetApiBase()->GetApi(ORT_API_VERSION)->GetAllocatorWithDefaultOptions(&RawAllocator));

    // Populate InputNames
    char* inputName = nullptr;
    Ort::ThrowOnError(OrtGetApiBase()->GetApi(ORT_API_VERSION)->SessionGetInputName(*OrtSession, 0, RawAllocator, &inputName));
    InputNameStrs.emplace_back(inputName);
    InputNames.Add(InputNameStrs.back().c_str());
    Ort::GetApi().AllocatorFree(RawAllocator, inputName);
//
//    Ort::AllocatorWithDefaultOptions allocator;
//    size_t outputCount = OrtSession->GetOutputCount();
//
//    for (size_t i = 0; i < outputCount; ++i)
//    {
//        char* outputName = nullptr;
//        OrtStatus* status = Ort::GetApi().SessionGetOutputName(*OrtSession, i, allocator, &outputName);
//        if (status != nullptr)
//        {
//            const char* msg = Ort::GetApi().GetErrorMessage(status);
//            UE_LOG(LogTemp, Error, TEXT("❌ Failed to get output name [%zu]: %s"), i, *FString(msg));
//            Ort::GetApi().ReleaseStatus(status);
//            continue;
//        }
//
//        // ✅ Safe: copy the name
//        std::string CopiedName(outputName);
//        OutputNameStrs.emplace_back(CopiedName);                      // Keep ownership
//        OutputNames.Add(OutputNameStrs.back().c_str());               // Use valid pointer
//        UE_LOG(LogTemp, Warning, TEXT("✔ Output name [%zu]: %s"), i, *FString(CopiedName.c_str()));
//        Ort::GetApi().AllocatorFree(allocator, outputName);           // Free heap-allocated name
//    }


    std::string lastValidName = "1965";
    OutputNameStrs = { lastValidName };
    OutputNames.Empty();
    OutputNames.Add(OutputNameStrs.back().c_str());

    UE_LOG(LogTemp, Warning, TEXT("Using only outpuuuuuuuuuut: %s"), *FString(lastValidName.c_str()));



    UE_LOG(LogTemp, Warning, TEXT("Total OutputNames = %d"), OutputNames.Num());


    UE_LOG(LogTemp, Warning, TEXT("Total OutputNameshjhgjfhgjfghjfhgjfhgjfhg = %d"), OutputNames.Num());
    UE_LOG(LogTemp, Warning, TEXT("Total 7676776767676776767 = %d"), OutputNames.Num());


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
    Ort::AllocatorWithDefaultOptions allocator;

    const int Width = 320, Height = 320;
    std::vector<float> InputTensor(1 * 3 * Height * Width);
    int idx = 0;
    for (int c = 0; c < 3; ++c)
        for (int y = 0; y < Height; ++y)
            for (int x = 0; x < Width; ++x)
                InputTensor[idx++] = resized.at<cv::Vec3f>(y, x)[c];

    if (OutputNames.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("OutputNames is empty — skipping inference."));
        return;
    }

    Ort::MemoryInfo MemInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<int64_t> InputShape = {1, 3, Height, Width};
    Ort::Value InputTensorVal = Ort::Value::CreateTensor<float>(MemInfo, InputTensor.data(), InputTensor.size(), InputShape.data(), InputShape.size());

    std::vector<Ort::Value> OutputTensors;
    UE_LOG(LogTemp, Warning, TEXT("Total 767677676767677676jhghjfhg7 = %d"), OutputNames.Num());
    try
    {
        for (int i = 0; i < InputNames.Num(); ++i)
        {
            FString NameStr(InputNames[i]);
            UE_LOG(LogTemp, Warning, TEXT("✔ InputNameeeeeeeee[%d] = %s"), i, *NameStr);
        }
        
        for (int i = 0; i < OutputNames.Num(); ++i)
        {
            FString NameStr(OutputNames[i]);
            UE_LOG(LogTemp, Warning, TEXT("✔ OutputnnnnnnnnnnnnName[%d] = %s"), i, *NameStr);
        }


        OutputTensors = OrtSession->Run(
            Ort::RunOptions{nullptr},
            InputNames.GetData(), &InputTensorVal, 1,
            OutputNames.GetData(), OutputNames.Num());

        UE_LOG(LogTemp, Warning, TEXT("Inference succeeded. Output tensor count: %d"), (int)OutputTensors.size());

        float* OutData = OutputTensors.back().GetTensorMutableData<float>();
        OutputBuffer.assign(OutData, OutData + Height * Width);

        auto shape = OutputTensors.back().GetTensorTypeAndShapeInfo().GetShape();
        UE_LOG(LogTemp, Warning, TEXT("Output Shape = [%lld, %lld, %lld, %lld]"),
            shape[0], shape[1], shape[2], shape[3]);
    }
    catch (const Ort::Exception& e)
    {
        UE_LOG(LogTemp, Error, TEXT("ONNX Inference failed: %s"), *FString(e.what()));
        return;
    }


    
    
    
    
//    OutputBuffer.assign(OutData, OutData + Height * Width);
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
        UE_LOG(LogTemp, Warning, TEXT("in if of frameeeeeeeeeeeee.channels() : %d"), frame.channels());
        cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA);
    }
    int cx = alphaMask.cols / 2;
    int cy = alphaMask.rows / 2;
    uchar centerAlpha = alphaMask.at<uchar>(cy, cx);
    UE_LOG(LogTemp, Warning, TEXT("[Alpha Debug] Centereeeerrrrrrrrrrr alpha: %d"), centerAlpha);
    for (int y = 0; y < frame.rows; ++y)
    {
        for (int x = 0; x < frame.cols; ++x)
        {
            uchar alpha = alphaMask.at<uchar>(y, x);
            cv::Vec4b& px = frame.at<cv::Vec4b>(y, x);
            px[3] = (alpha < 64) ? 0 : alpha;
//            px[3] = 255;
        }
    }
    double minVal, maxVal;
    cv::minMaxLoc(alphaMask, &minVal, &maxVal);
    UE_LOG(LogTemp, Warning, TEXT("Alpha mask rangerrtttttttttttt: min = %f, max = %f"), minVal, maxVal);

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
        //UE_LOG(LogTemp, Warning, TEXT("SrcPitch = %d"), VideoSize.X * sizeof(FColor));

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
