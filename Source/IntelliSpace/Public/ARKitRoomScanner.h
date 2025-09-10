// ARKitRoomScanner.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "ImagePlateComponent.h"
#include "ARKitRoomScanner.generated.h"

// Forward declarations
namespace cv {
    class Mat;
}

USTRUCT(BlueprintType)
struct FARMeshData
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadOnly)
    TArray<FVector> Vertices;
    
    UPROPERTY(BlueprintReadOnly)
    TArray<int32> Triangles;
    
    UPROPERTY(BlueprintReadOnly)
    TArray<FVector> Normals;
    
    UPROPERTY(BlueprintReadOnly)
    int32 VertexCount = 0;
    
    UPROPERTY(BlueprintReadOnly)
    int32 TriangleCount = 0;
};

UCLASS()
class INTELLISPACE_API AARKitRoomScanner : public AActor
{
    GENERATED_BODY()
    
public:
    AARKitRoomScanner();
    virtual ~AARKitRoomScanner();
    
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;
    
    UFUNCTION(BlueprintCallable, Category = "ARKit")
    void StartScanning();
    
    UFUNCTION(BlueprintCallable, Category = "ARKit")
    void StopScanning();
    
    UFUNCTION(BlueprintCallable, Category = "ARKit")
    void ClearMesh();
    
    UFUNCTION(BlueprintCallable, Category = "ARKit")
    void ExportMesh(const FString& FileName);
    
    UFUNCTION(BlueprintCallable, Category = "ARKit")
    void ToggleScanning();
    
    UFUNCTION(BlueprintCallable, Category = "ARKit")
    FString GetStatusText() const;
    
    UPROPERTY(BlueprintReadOnly, Category = "ARKit")
    bool bIsScanning;
    
    UPROPERTY(BlueprintReadOnly, Category = "ARKit")
    FARMeshData CurrentMeshData;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARKit")
    UProceduralMeshComponent* ProceduralMesh;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARKit")
    UImagePlateComponent* CameraPreviewPlate;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARKit")
    UMaterialInterface* MeshMaterial;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARKit")
    float MeshUpdateInterval = 0.5f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> HUDWidgetClass;
    
protected:
    UPROPERTY()
    UTexture2D* CameraTexture;
    
    UPROPERTY()
    UMaterialInstanceDynamic* CameraMaterial;
    
    UPROPERTY()
    class UScannerHUD* HUDWidget;
    
private:
    void* ARSession;
    void* ARDelegate;
    float TimeSinceLastUpdate;
    
    cv::Mat* cameraFrame;
    TArray<FColor> CameraColorData;
    
    void UpdateMeshFromARKit();
    void UpdateCameraFrame();
    void CreateHUD();
};
