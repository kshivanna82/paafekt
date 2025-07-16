#include "FurniLife.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"


#include "Engine/Texture2D.h"
#include "RenderUtils.h"
#include "RHICommandList.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Rendering/Texture2DResource.h"
#include "NNERuntimeCPU.h"
#include "Templates/UniquePtr.h"
#include "NNERuntime.h"

#include "NNE.h"
//#include "NNEUtilitiesModelData.h"

#if PLATFORM_MAC
#include "MacNNERunner.h"
#elif PLATFORM_IOS
#include "IOSNNERunner.h"
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#endif

AFurniLife::AFurniLife(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    UE_LOG(LogTemp, Warning, TEXT("AFurniLife Constructor"));
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

FString AFurniLife::LoadFileToString(FString Filename) {
    FString directory = FPaths::ProjectContentDir();
    FString result;
    IPlatformFile& file = FPlatformFileManager::Get().GetPlatformFile();
    if(file.CreateDirectory(*directory)) {
        FString myFile = directory + "/" + Filename;
        FFileHelper::LoadFileToString(result, *myFile);
    }
    return result;
}

void AFurniLife::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Warning, TEXT("AFurniLife BeginPlay"));
    cap.open(CameraID);
    if (!cap.isOpened())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to open camera %d"), CameraID);
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
    
    FString FolderPath = FPaths::ProjectContentDir();
    //    FString FolderPath = FPaths::ProjectContentDir() / TEXT("/Models/");
    
   IFileManager& FileManager = IFileManager::Get();
   TArray<FString> FoundFiles;

   FileManager.FindFiles(FoundFiles, *FolderPath, TEXT("*.*")); // List all files

   for (const FString& File : FoundFiles)
   {
       UE_LOG(LogTemp, Log, TEXT("Foundvvvvvvvvvvvvvvvv file in Models/: %s"), *File);
   }
    
    

#if PLATFORM_IOS
    NeuralRunner = NewObject<UIOSNNERunner>();
#else
    NeuralRunner = NewObject<UMacNNERunner>();
#endif

    FString ModelPath = FPaths::ProjectContentDir() / TEXT("Models/U2NET1.onnx");
    NeuralRunner->InitializeModel(ModelPath);
}

void AFurniLife::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    RefreshTimer += DeltaTime;
    if (isStreamOpen && RefreshTimer >= 1.0f / RefreshRate)
    {
        RefreshTimer -= 1.0f / RefreshRate;
        AsyncTask(ENamedThreads::GameThread, [this]()
        {
            if (ReadFrame())
            {
                // Delay the rendering-related logic by a few milliseconds to stay on render/game thread
                GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AFurniLife::OnNextVideoFrame);
            }
            
            //        ReadFrameee();
            //        OnNextVideoFrame();
        });
    }
}

void AFurniLife::ProcessInputForModel()
{
    cv::resize(frame, resized, cv::Size(320, 320));

    cv::cvtColor(resized, resized, cv::COLOR_BGR2RGB);
    resized.convertTo(resized, CV_32FC3, 1.0 / 255.0);
}

void AFurniLife::ApplySegmentationMask()
{
//    cv::resize(alphaMask, alphaMask, cv::Size(VideoSize.X, VideoSize.Y));  // Resize back to full resolution
    const int Width = 320;
    const int Height = 320;

    const float* MaskOutput = OutputTensors.Last().GetData();
    cv::Mat mask(Height, Width, CV_32FC1, (void*)MaskOutput);
//    cv::Mat normalizedMask;
//
//    cv::normalize(mask, normalizedMask, 0.0, 1.0, cv::NORM_MINMAX);
//    normalizedMask.convertTo(alphaMask, CV_8UC1, 255.0);
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
}


// Declaration for UpdateTextureRegions helper
static void UpdateTextureRegions(
    UTexture2D* Texture,
    int32 MipIndex,
    uint32 NumRegions,
    FUpdateTextureRegion2D* Regions,
    uint32 SrcPitch,
    uint32 SrcBpp,
    uint8* SrcData,
    bool bFreeData
)
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

        FUpdateContext* Context = new FUpdateContext{ Texture, MipIndex, Regions, SrcData, SrcPitch, SrcBpp, bFreeData };

        ENQUEUE_RENDER_COMMAND(UpdateTextureRegionsData)(
            [Context](FRHICommandListImmediate& RHICmdList)
            {
                FTexture2DResource* TextureResource = static_cast<FTexture2DResource*>(Context->Tex->GetResource());
                RHIUpdateTexture2D(
                    TextureResource->GetTexture2DRHI(),
                    Context->Mip,
                    *Context->Region,
                    Context->Pitch,
                    Context->Data
                );

                if (Context->bFree)
                {
                    FMemory::Free(Context->Data);
                }
                delete Context;
            }
        );
    }
}


bool AFurniLife::ReadFrame()
{
    if (!Camera_Texture2D || !cap.isOpened())
        return false;

    // Read the camera frame
    cap.read(frame);
    
    
    
    if (frame.empty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Camera frame is empty"));
        return false;
    }
    //Neural
    
    if (!Camera_Texture2D || !cap.isOpened())
        return false;

    cap.read(frame);
    if (frame.empty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Camera frame is empty"));
        return false;
    }
    
    ProcessInputForModel();
    
    
    // Run the model via platform-specific NeuralRunner
    const int Width = 320;
    const int Height = 320;

    TArray<float> InputTensor;
    InputTensor.SetNum(1 * 3 * Width * Height);
    int idx = 0;
    for (int c = 0; c < 3; ++c)
        for (int y = 0; y < Height; ++y)
            for (int x = 0; x < Width; ++x)
                InputTensor[idx++] = resized.at<cv::Vec3f>(y, x)[c];

    TArray<float> OutMaskTensor;
    if (NeuralRunner && NeuralRunner->RunInference(InputTensor, Width, Height, OutMaskTensor, OutputTensors, OutputBindings))
    {
//        OutputTensors.SetNum(1);
//        OutputTensors[0] = MoveTemp(OutMaskTensor);
        UE_LOG(LogTemp, Log, TEXT("Inference succeeded."));
        ApplySegmentationMask();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Model inference failed"));
    }

    // Convert to BGRA (for Unreal)
    cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA);
    // Copy frame data to ColorData
//    FMemory::Memcpy(ColorData.GetData(), frame.data, ColorData.Num() * sizeof(FColor));
    
    for (int y = 0; y < frame.rows; ++y)
    {
        for (int x = 0; x < frame.cols; ++x)
        {
            cv::Vec4b pixel = frame.at<cv::Vec4b>(y, x);
            ColorData[y * frame.cols + x] = FColor(pixel[2], pixel[1], pixel[0], pixel[3]); // BGRA -> RGBA
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
