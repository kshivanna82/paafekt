#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#ifdef check
#undef check
#endif
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgcodecs.hpp>
#define check(expr) UE_CHECK_IMPL(expr)

#include "ImagePlateComponent.h"
#include "ProceduralMeshComponent.h"

#include "Templates/UniquePtr.h"
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
#include "CoreMLModelBridge.h"
#endif

#include <deque>
#include <vector>
#include <algorithm>

#include "FurniLife.generated.h"

UCLASS()
class INTELLISPACE_API AFurniLife : public AActor
{
    GENERATED_BODY()

public:
    AFurniLife(const FObjectInitializer& ObjectInitializer);
    virtual void Tick(float DeltaTime) override;
    
    UFUNCTION(BlueprintImplementableEvent, Category = Camera)
    void OnNextVideoFrame();
    
    UFUNCTION(BlueprintCallable, Category = Camera)
    bool ReadFrame();
    
    // Camera Properties
    UPROPERTY(BluePrintReadWrite, EditAnywhere, Category = Camera, meta = (ClampMin = 0, UIMin = 0))
    int32 CameraID;
    
    UPROPERTY(BluePrintReadWrite, EditAnywhere, Category = Camera, meta = (ClampMin = 0, UIMin = 0))
    int32 VideoTrackID;
    
    UPROPERTY(BluePrintReadWrite, EditAnywhere, Category = Camera, meta = (ClampMin = 0, UIMin = 0))
    float RefreshRate;
    
    UPROPERTY(BluePrintReadWrite, EditAnywhere, Category = Camera)
    FVector2D VideoSize;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    bool isStreamOpen;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    float RefreshTimer;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
    float Brightness;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
    float Multiply;
    
    // Media Components
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
    UMediaPlayer* Camera_MediaPlayer;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
    UMediaTexture* Camera_MediaTexture;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
    UTextureRenderTarget2D* Camera_RenderTarget;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
    UMaterialInstanceDynamic* Camera_MatPost;
    
    // Scene Components
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
    USceneComponent* rootComp;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
    UImagePlateComponent* ImagePlatePost;
    
    // Texture Components
    UPROPERTY(BlueprintReadWrite, VisibleAnyWhere, Category = Camera)
    UTexture2D* Camera_Texture2D;
    
    UPROPERTY()
    UTexture2D* VideoMask_Texture2D;
    
    UPROPERTY(BlueprintReadWrite, VisibleAnyWhere, Category = Camera)
    TArray<FColor> ColorData;
    
    // Room mesh components
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Room")
    UProceduralMeshComponent* RoomMesh;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Room")
    FString DefaultOBJFile = TEXT("testtt");
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
    float RoomMeshOpacity = 0.3f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
    bool bShowRoomMesh = true;
    
    UFUNCTION(BlueprintCallable, Category = "Room")
    void LoadRoomMeshFromFile(const FString& OBJFileName);
    
    UFUNCTION(BlueprintCallable, Category = "Room")
    void ToggleRoomMeshVisibility();
    
    // Static instance
    static AFurniLife* CurrentInstance;
    
    // iOS specific callback
    void OnCameraFrameFromPixelBuffer(CVPixelBufferRef buffer);

protected:
    virtual void BeginPlay() override;

private:
    // OpenCV members
    cv::VideoCapture cap;
    cv::Mat frame;
    cv::Mat resized;
    cv::Mat alphaMask;
    cv::Size cvSize;
    cv::Mat cvMat;
    
    // Private methods
    void InitializeCamera();
    void ProcessInputForModel();
    void RunModelInference();
    void ApplySegmentationMask();
    void UpdateTextureSafely();
    void AssignTextureToImagePlate();
    void UpdateImagePlateTexture();
    
    uchar CalculateSmartThreshold(const cv::Mat& mask);
    uchar CalculateOtsuThreshold(const cv::Mat& mask);
    uchar CalculatePercentileThreshold(const cv::Mat& mask);
    uchar CalculateHistogramThreshold(const cv::Mat& mask);
    uchar CalculateAdaptiveThreshold(const cv::Mat& mask);
    float CalculateSegmentationConfidence(const cv::Mat& mask, uchar threshold);
    uchar ApplyTemporalSmoothing(uchar currentThreshold);
    
    // Member variables for threshold calculation
    bool bDebugMode = false;
    std::deque<uchar> thresholdHistory;
    float smoothedThreshold = -1.0f;
    
    void UpdateImagePlatePosition(const cv::Rect& personBounds, int maskWidth, int maskHeight);
    
    // OBJ parsing
    bool ParseOBJFile(const FString& FilePath, TArray<FVector>& OutVertices,
                      TArray<int32>& OutTriangles, TArray<FVector>& OutNormals);
    
#if PLATFORM_MAC
    TWeakInterfacePtr<INNERuntimeCPU> CpuRuntime;
    TSharedPtr<UE::NNE::IModelCPU> CpuModel;
    TSharedPtr<UE::NNE::IModelInstanceCPU> CpuModelInstance;
    
    TArray<TArray<float>> OutputTensors;
    TArray<float> OutputBuffer;
    TArray<UE::NNE::FTensorBindingCPU> OutputBindings;
#endif

#if PLATFORM_IOS
    std::vector<float> OutputBuffer;
#endif
};
