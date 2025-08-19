#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ISUserDataSaveGame.generated.h"

UCLASS()
class INTELLISPACERUNTIME_API UISUserDataSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category="IS|UserData")
    FString UserName;

    UPROPERTY(BlueprintReadWrite, Category="IS|UserData")
    FString Phone;
};
