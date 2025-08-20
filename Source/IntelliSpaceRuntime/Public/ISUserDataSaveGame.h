#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ISUserDataSaveGame.generated.h"

UCLASS()
class INTELLISPACERUNTIME_API UISUserDataSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    // Slot name and index you can reuse elsewhere
    static constexpr const TCHAR* SlotName = TEXT("FirstRunUserData");
    static constexpr int32 UserIndex = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
    FString UserName;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
    FString PhoneNumber;
};
