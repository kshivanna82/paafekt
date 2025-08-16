#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "SegDecorActor.generated.h"

UENUM(BlueprintType)
enum class ERoomPreset : uint8 { LivingRoom, MasterBedroom, Kitchen };

UCLASS()
class ASegDecorActor : public AActor
{
    GENERATED_BODY()
public:
    ASegDecorActor();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    void OnCameraFrame(void* PixelBuffer);

    UFUNCTION(BlueprintCallable) void SetRoomPreset(ERoomPreset Preset);
    UFUNCTION(BlueprintCallable) void LaunchBuilderMode();

private:
    void UpdateSegmentation(void* PixelBuffer);
    void ApplyMaskToMaterial(const TArray<float>& Mask, int W, int H);

    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* RoomMesh;
    UPROPERTY(EditAnywhere, Category="Room") ERoomPreset CurrentPreset = ERoomPreset::LivingRoom;
    UPROPERTY(EditAnywhere, Category="Materials") UMaterialInterface* SegCompositeMaterial = nullptr;
    UMaterialInstanceDynamic* SegMID = nullptr;

    UPROPERTY(EditAnywhere, Category="UI") TSubclassOf<UUserWidget> RightDrawerWidgetClass;
    UUserWidget* RightDrawerWidget = nullptr;
};
