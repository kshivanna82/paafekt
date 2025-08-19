#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ISUserDataSaveGame.generated.h"

UCLASS()
class INTELLISPACERUNTIME_API UISUserDataSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="User")
	FString Name;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="User")
	FString Phone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="User")
	bool bCompleted = false;
};
