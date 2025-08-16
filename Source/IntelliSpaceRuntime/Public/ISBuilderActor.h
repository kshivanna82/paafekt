#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Blueprint/UserWidget.h"
#include "ISBuilderActor.generated.h"

UENUM()
enum class ECaptureState : uint8 { Capturing, WaitingConfirm, Processing, Reviewing };

UCLASS()
class AISBuilderActor : public AActor
{
    GENERATED_BODY()
public:
    AISBuilderActor();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable) void SaveCurrentMeshOBJ(const FString& Name = TEXT("MiDaSMesh.obj"));
    UFUNCTION(BlueprintCallable) void DiscardCurrentCapture();

    void OnCameraFrameFromPixelBuffer(void* PixelBuffer);

    UFUNCTION() void OnReview_Export();
    UFUNCTION() void OnReview_Retake();
    UFUNCTION() void OnReview_Discard();

#if WITH_DEV_AUTOMATION_TESTS
    void __Test_BuildDummyAndExport();
#endif

protected:
    void UpdateDepthAndMesh(void* InputFrame);
    void ShowScanPrompt(); void HideScanPrompt();
    UFUNCTION() void OnScanPrompt_BuildNow();
    UFUNCTION() void OnScanPrompt_KeepScanning();
    UFUNCTION() void OnScanPrompt_Reset();
    UFUNCTION() void OnScanPrompt_Discard();
    void ShowReviewUI(); void HideReviewUI();
    void StartCamera(); void StopCamera();

    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* RoomMesh = nullptr;

    UPROPERTY(EditAnywhere, Category="Depth") int32 FrameInterval = 1;
    UPROPERTY(EditAnywhere, Category="Capture") int32 TargetFrameCount = 45;
    UPROPERTY(EditAnywhere, Category="Capture") float TargetSeconds = 5.0f;
    int32 SessionFrameCount = 0; double SessionStartTime = 0.0;

    int32 FrameCounter = 0;
    ECaptureState State = ECaptureState::Capturing;

    void* LastFrame = nullptr;

    UPROPERTY(EditAnywhere, Category="UI") TSubclassOf<UUserWidget> ReviewWidgetClass;
    UUserWidget* ReviewWidget = nullptr;

    UPROPERTY(EditAnywhere, Category="UI") TSubclassOf<UUserWidget> ScanPromptWidgetClass;
    UUserWidget* ScanPromptWidget = nullptr;

    TArray<FVector> LastVerts; TArray<int32> LastTris;
};
