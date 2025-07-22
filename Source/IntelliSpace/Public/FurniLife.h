#pragma once
#ifdef check
#undef check
#endif
#include "opencv2/imgproc.hpp"
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/videoio.hpp"
#include <opencv2/imgcodecs.hpp>
#define check(expr) UE_CHECK_IMPL(expr)
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ImagePlateComponent.h"

#include "Templates/UniquePtr.h"
#include "UObject/Class.h"
#include "UObject/Object.h"
#include "UObject/UObjectBaseUtility.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "Runtime/Engine/Classes/Engine/Texture2D.h"
#include "Runtime/Engine/Classes/Engine/TextureRenderTarget2D.h"
#include <Runtime/Core/Public/Misc/FileHelper.h>

#if PLATFORM_MAC
#include "NNE.h"
#include "NNERuntime.h"
#include "NNERuntimeCPU.h"
#include "NNEModelData.h"
#endif
#if PLATFORM_IOS
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#endif

#include "FurniLife.generated.h"


UCLASS()
class INTELLISPACE_API AFurniLife : public AActor
{
    GENERATED_BODY()
public:
    AFurniLife(const FObjectInitializer& ObjectInitializer);
protected:
    virtual void BeginPlay() override;
public:
    virtual void Tick(float DeltaTime) override;
    UFUNCTION(BlueprintImplementableEvent, Category = Camera)
        void OnNextVideoFrame();
    UFUNCTION(BlueprintCallable, Category = Camera)
        bool ReadFrame();
    
    
    
    UPROPERTY(BluePrintReadWrite, EditAnywhere, Category = Camera, meta = (ClampMin = 0, UIMin = 0))
        int32 CameraID;
    UPROPERTY(BluePrintReadWrite, EditAnywhere, Category = Camera, meta = (ClampMin = 0, UIMin = 0))
        int32 VideoTrackID;
        
    UPROPERTY(BluePrintReadWrite, EditAnywhere, Category = Camera, meta = (ClampMin = 0, UIMin = 0))
        float RefreshRate;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
        FVector2D VideoSize;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
        bool isStreamOpen = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
        float RefreshTimer;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
        float Brightness;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
        float Multiply;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
        UMediaPlayer* Camera_MediaPlayer;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
        UMediaTexture* Camera_MediaTexture;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
        UTextureRenderTarget2D* Camera_RenderTarget;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
        UMaterialInstanceDynamic* Camera_MatRaw;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
        UMaterialInstanceDynamic* Camera_MatPost;
    
        
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
        USceneComponent* rootComp;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
        UImagePlateComponent* ImagePlateRaw;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
        UImagePlateComponent* ImagePlatePost;
    UPROPERTY(BlueprintReadWrite, VisibleAnyWhere, Category = Camera)
        UTexture2D* Camera_Texture2D;
    UPROPERTY()
        UTexture2D* VideoMask_Texture2D;
    UPROPERTY(BlueprintReadWrite, VisibleAnyWhere, Category = Camera)
        TArray<FColor> ColorData;
    



    cv::VideoCapture cap;
    cv::Mat frame;
    cv::Mat resized;
    cv::Mat alphaMask;
    cv::Size cvSize;
    cv::Mat cvMat;
    
    void ProcessInputForModel();
    void RunModelInference();
    void ApplySegmentationMask();
    
#if PLATFORM_MAC
    TWeakInterfacePtr<INNERuntimeCPU> CpuRuntime;
    TSharedPtr<UE::NNE::IModelCPU> CpuModel;
    TSharedPtr<UE::NNE::IModelInstanceCPU> CpuModelInstance;
    
    TArray<TArray<float>> OutputTensors;
    TArray<float> OutputBuffer;

    TArray<UE::NNE::FTensorBindingCPU> OutputBindings;
#endif
#if PLATFORM_IOS

    Ort::Env OrtEnv{ORT_LOGGING_LEVEL_WARNING, "FurniLife"};
    Ort::SessionOptions SessionOptions;
    Ort::Session* OrtSession = nullptr;

    std::vector<std::string> InputNameStrs;
    std::vector<std::string> OutputNameStrs;
    TArray<const char*> InputNames;
    TArray<const char*> OutputNames;
    //int32 FrameCounter;
//    FString OutputDir;
//
//    std::vector<const char*> InputNames;
//    std::vector<const char*> OutputNames;
    std::vector<float> OutputBuffer;

#endif
    
    
};
