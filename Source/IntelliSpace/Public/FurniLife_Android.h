#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FurniLife_Android.generated.h"

class UProceduralMeshComponent;
class UImagePlateComponent;
class UUserWidget;

UCLASS()
class INTELLISPACE_API AFurniLife_Android : public AActor
{
    GENERATED_BODY()
    
public:
    AFurniLife_Android();
    
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FString DefaultOBJFile = TEXT("testtt");
    
protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* rootComp;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UProceduralMeshComponent* RoomMesh;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UImagePlateComponent* ImagePlatePost;
    
    UPROPERTY()
    UUserWidget* MenuWidget;
    
    void LoadRoomMeshFromFile(const FString& FileName);
    void CreateMenuButton();
    
    UFUNCTION()
    void OnMenuButtonClicked();
    
#if PLATFORM_ANDROID
    void InitializeCamera();
    void RequestCameraPermission();
    void ProcessCameraFrame();
    void InitializeTFLite();
    void RunSegmentation();
    
private:
    void* CameraSession;
    void* TFLiteModel;
    void* TFLiteInterpreter;
    bool bIsCameraActive;
    
    TArray<uint8> CameraBuffer;
    class UTexture2D* CameraTexture;
#endif
    
    static AFurniLife_Android* CurrentInstance;
};
