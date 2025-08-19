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
    virtual void OnStart() override;

private:
    // Called after any map finishes loading (PIE and packaged)
    void OnPostLoadMapWithWorld(UWorld* LoadedWorld);

    // Show/hide the first-run UI
    void AddFirstRunUI();
    void RemoveFirstRunUI();

    // Delegate handler from the first-run UI
    UFUNCTION()
    void OnFirstRunSubmitted(const FString& Name, const FString& Phone);

    // Persistence helpers
    bool LoadUserData(FString& OutName, FString& OutPhone) const;
    bool SaveUserData(const FString& Name, const FString& Phone) const;

private:
    // Keep a reference so we can remove it later
    TSharedPtr<class SWidget> FirstRunUI;

    // Save slot info
    static constexpr const TCHAR* FirstRunSlot = TEXT("IS_FirstRunUserData");
    static constexpr int32 UserIndex = 0;
};
