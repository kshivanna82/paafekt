#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ModeSubsystem.generated.h"

UENUM(BlueprintType)
enum class EISAppMode : uint8
{
    SegAndDecor        UMETA(DisplayName="Segment & Decor"),
    Builder3D          UMETA(DisplayName="3D Builder"),
    FutureInteriorDesign UMETA(DisplayName="Future: Interior Design")
};

UCLASS()
class UModeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Coll) override { CurrentMode = EISAppMode::SegAndDecor; }
    UFUNCTION(BlueprintCallable) void SetMode(EISAppMode M) { CurrentMode = M; UE_LOG(LogIntelliSpace, Log, TEXT("[Mode] Switched to %d"), (int)M); }
    UFUNCTION(BlueprintCallable) EISAppMode GetMode() const { return CurrentMode; }
private:
    EISAppMode CurrentMode;
};
