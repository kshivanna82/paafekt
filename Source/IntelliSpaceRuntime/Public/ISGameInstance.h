#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ISGameInstance.generated.h"

/** GameInstance that shows the first-run form and stores Name/Phone to a SaveGame slot. */
UCLASS()
class INTELLISPACERUNTIME_API UISGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
//    virtual void Init() override;
    virtual void OnStart() override;
//    virtual void Shutdown() override;

private:
	void ShowFirstRun();
	void OnFirstRunSubmitted(const FString& Name, const FString& Phone);
	bool HasCompletedFirstRun() const;

private:
	TSharedPtr<SWidget> FirstRunWidget;
};
