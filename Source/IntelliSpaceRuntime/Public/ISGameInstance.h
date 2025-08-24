#pragma once

#include "Engine/GameInstance.h"
#include "ISGameInstance.generated.h"

class SWindow;

UCLASS()
class INTELLISPACERUNTIME_API UISGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    virtual void OnStart() override;

private:
    void ShowFirstRunIfNeeded();
    void OnFirstRunSubmitted();
    void EnsureCameraView();
    bool IsFirstRun() const;

private:
    TSharedPtr<SWindow> FirstRunWindow;
};
