#pragma once

#include "Materials/MaterialInstanceDynamic.h"
#include "opencv2/imgproc.hpp"
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/videoio.hpp"

#include "NNE.h"
#include "NNEAttributeMap.h"
#include "NNEModelData.h"
#include "NNEModelOptimizerInterface.h"

#include "Templates/UniquePtr.h"

#include "NNERuntime.h"
#include "NNERuntimeCPU.h"
#include "NNERuntimeGPU.h"
#include "PlatformNNERunner.h"

#include "Templates/UniquePtr.h"
#include "UObject/Class.h"
#include "UObject/Object.h"
#include "UObject/UObjectBaseUtility.h"


#include "ImagePlateComponent.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "Runtime/Engine/Classes/Engine/Texture2D.h"
#include "Runtime/Engine/Classes/Engine/TextureRenderTarget2D.h"

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"
#include "Templates/SharedPointer.h"
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
    
    UPROPERTY(BluePrintReadWrite, EditAnywhere, Category = Camera)
        USceneComponent* rootComp;
    UPROPERTY(BluePrintReadWrite, EditAnywhere, Category = Camera)
        UStaticMeshComponent* Screen_Raw;
    UPROPERTY(BluePrintReadWrite, EditAnywhere, Category = Camera)
        UStaticMeshComponent* Screen_Post;
    
    UPROPERTY(BluePrintReadWrite, EditAnywhere, Category = Camera, meta = (ClampMin = 0, UIMin = 0))
        int32 CameraID;
    UPROPERTY(BluePrintReadWrite, EditAnywhere, Category = Camera, meta = (ClampMin = 0, UIMin = 0))
        int32 VideoTrackID;
    
    UPROPERTY(BluePrintReadWrite, EditAnywhere, Category = Camera, meta = (ClampMin = 0, UIMin = 0))
        float RefreshRate;
    UPROPERTY(BluePrintReadWrite, Category = Camera)
        float RefreshTimer;
    UPROPERTY(BluePrintReadWrite, Category = Camera)
        bool isStreamOpen;
    UPROPERTY(BluePrintReadWrite, Category = Camera)
        FVector2D VideoSize;
    
    UPROPERTY(BluePrintReadWrite, EditAnywhere, Category = Camera)
        float Brightness;
    UPROPERTY(BluePrintReadWrite, EditAnywhere, Category = Camera)
        float Multiply;
    
    UPROPERTY(BluePrintReadWrite, EditAnywhere, Category = Camera)
        UMediaPlayer* Camera_MediaPlayer;
    UPROPERTY(BluePrintReadWrite, EditAnywhere, Category = Camera)
        UMediaTexture* Camera_MediaTexture;
    UPROPERTY(BluePrintReadWrite, EditAnywhere, Category = Camera)
        UTextureRenderTarget2D* Camera_RenderTarget;
    UPROPERTY(BluePrintReadWrite, EditAnywhere, Category = Camera)
        UMaterialInstanceDynamic* Camera_MatRaw;
    UPROPERTY(BluePrintReadWrite, EditAnywhere, Category = Camera)
        UMaterialInstanceDynamic* Camera_MatPost;
    
    UPROPERTY(BluePrintReadWrite, VisibleAnyWhere, Category = Camera)
        UTexture2D* Camera_Texture2D;
    UPROPERTY(BluePrintReadWrite, VisibleAnyWhere, Category = Camera)
        TArray<FColor> ColorData;
    

    
    UPROPERTY(EditAnywhere, Category = "Inference")
    TObjectPtr<UNNEModelData> ModelData;

    TWeakInterfacePtr<INNERuntimeCPU> CpuRuntime;
    TSharedPtr<UE::NNE::IModelCPU> CpuModel;
    TSharedPtr<UE::NNE::IModelInstanceCPU> CpuModelInstance;

    UPROPERTY()
    UMaterialInstanceDynamic* PostMaterialInstance;
    
    UPROPERTY()
    UImagePlateComponent* ImagePlateRaw;

    UPROPERTY()
    UImagePlateComponent* ImagePlatePost;

    UPROPERTY()
    UTexture2D* VideoMask_Texture2D;

    UPROPERTY()
    UPlatformNNERunner* NeuralRunner;

    cv::Mat resized;
    cv::Mat alphaMask;
    TArray<TArray<float>> OutputTensors;
//    #if PLATFORM_MAC
    TArray<UE::NNE::FTensorBindingCPU> OutputBindings;
//    #endif


    UFUNCTION(BluePrintImplementableEvent, Category = Camera)
        void OnNextVideoFrame();
    UFUNCTION(BluePrintCallable, Category = Camera)
        bool ReadFrame();
    UFUNCTION(BluePrintCallable, Category = "File I/O")
        static FString LoadFileToString(FString Filename);
    
    void ProcessInputForModel();
    void ApplySegmentationMask();
    
    cv::Size cvSize;
    cv::Mat cvMat;
    
    cv::VideoCapture cap;
    cv::Mat frame;
};
