// MidasDepth.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

// Forward declare OpenCV types
namespace cv {
    class VideoCapture;
    class Mat;
    template<typename T> class Size_;
    typedef Size_<int> Size;
}

#include <vector>
#include <deque>
#include "MidasDepth.generated.h"

class UImagePlateComponent;
class USceneComponent;

UCLASS()
class INTELLISPACE_API AMidasDepth : public AActor
{
    GENERATED_BODY()
    
public:
    AMidasDepth(const FObjectInitializer& ObjectInitializer);
    virtual ~AMidasDepth();
    
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    
    static AMidasDepth* CurrentInstance;
    
protected:
    void InitializeCamera();
    bool ReadFrame();
    void ProcessDepthEstimation();
    void RunMidasInference();
    void ApplyDepthVisualization();
    void CreateDepthColorMap(const cv::Mat* depthMap, cv::Mat* colorMap);
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    USceneComponent* rootComp;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UImagePlateComponent* ImagePlateDepth;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* Camera_Texture2D;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* Depth_Texture2D;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UMaterialInstanceDynamic* Depth_Material;
    
    // Camera properties (using pointers to avoid including OpenCV in header)
    cv::VideoCapture* cap;
    cv::Mat* frame;
    cv::Mat* resized;
    cv::Mat* depthResult;
    cv::Mat* cvMat;
    cv::Size* cvSize;
    
    bool isStreamOpen;
    int CameraID;
    FVector2D VideoSize;
    
    // Color data for texture
    TArray<FColor> ColorData;
    
    // Model output
    std::vector<float> DepthOutputBuffer;
    
    // Visualization mode
    enum class DepthVisMode
    {
        Grayscale,
        Colormap,
        Overlay
    };
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 VisualizationMode = 1; // Default to Colormap
};
