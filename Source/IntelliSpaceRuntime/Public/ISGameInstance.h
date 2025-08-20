#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ISGameInstance.generated.h"

UCLASS()
class INTELLISPACERUNTIME_API UISGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    virtual void Init() override;

private:
    // Delegate target after a map loads
    void HandlePostLoadMap(UWorld* LoadedWorld);

    // First-run UI
    void AddFirstRunUI();
    void RemoveFirstRunUI();
    void OnFirstRunSubmitted(const FString& Name, const FString& Phone);

    // Camera setup if no pawn/view yet
    void EnsureCameraView(UWorld* World);

    // Slate root we add to the viewport
    TSharedPtr<class SWidget> FirstRunRoot;
};
