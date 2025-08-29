#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Templates/SubclassOf.h"   // <-- for TSubclassOf
#include "LoginUIBoot.generated.h"

class ULoginWidget;

UCLASS(BlueprintType)
class INTELLISPACERUNTIME_API ULoginUIBoot : public UObject
{
    GENERATED_BODY()

public:
    // Call once (from an Actor's BeginPlay or Level BP)
    UFUNCTION(BlueprintCallable, Category="Login")
    void Init(UWorld* World);

    UFUNCTION() void OnSendClicked(FString Phone);
    UFUNCTION() void OnVerifyClicked(FString Phone, FString Code);

    UFUNCTION(BlueprintCallable, Category="Login")
    void NavigateToSegDecor();

    // Assign WBP_Login (Parent Class MUST be ULoginWidget)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Login")
    TSubclassOf<ULoginWidget> LoginWidgetClass = nullptr;

private:
    UPROPERTY() ULoginWidget* LoginWidget = nullptr;
};
