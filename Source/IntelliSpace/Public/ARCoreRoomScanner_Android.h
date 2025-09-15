#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "ARCoreRoomScanner_Android.generated.h"

UCLASS()
class INTELLISPACE_API AARCoreRoomScanner_Android : public AActor
{
    GENERATED_BODY()
    
public:
    AARCoreRoomScanner_Android();
    
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    
    UFUNCTION(BlueprintCallable, Category = "ARCore")
    void StartScanning();
    
    UFUNCTION(BlueprintCallable, Category = "ARCore")
    void StopScanning();
    
    UFUNCTION(BlueprintCallable, Category = "ARCore")
    void ExportMesh(const FString& FileName);
    
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUserWidget> HUDWidgetClass;
    
protected:
    UPROPERTY(VisibleAnywhere)
    UProceduralMeshComponent* ProceduralMesh;
    
    void CreateHUD();
    
    UFUNCTION()
    void OnFurniMatchClicked();
    
    // Wrapper functions for button bindings
    UFUNCTION()
    void OnStartClicked() { StartScanning(); }
    
    UFUNCTION()
    void OnStopClicked() { StopScanning(); }
    
    UFUNCTION()
    void OnExportClicked() { ExportMesh(TEXT("testtt")); }
    
#if PLATFORM_ANDROID
    void InitializeARCore();
    void UpdatePlanesFromARCore();
#endif
    
private:
    bool bIsScanning;
    void* ARSession;
    
    TArray<FVector> MeshVertices;
    TArray<int32> MeshTriangles;
    TArray<FVector> MeshNormals;
    
    // Add this declaration
    void CreateMockRoomData();
};
