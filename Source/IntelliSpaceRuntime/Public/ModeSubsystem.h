#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ModeSubsystem.generated.h"

UENUM(BlueprintType)
enum class EISAppMode : uint8
{
    SegAndDecor           UMETA(DisplayName="SegAndDecor"),
    Builder3D             UMETA(DisplayName="Builder3D"),
    FutureInteriorDesign  UMETA(DisplayName="FutureInteriorDesign")
};

UCLASS()
class INTELLISPACERUNTIME_API UModeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) void SetMode(EISAppMode M) { CurrentMode = M; }
    UFUNCTION(BlueprintCallable) EISAppMode GetMode() const { return CurrentMode; }

private:
    UPROPERTY() EISAppMode CurrentMode = EISAppMode::SegAndDecor;
};
