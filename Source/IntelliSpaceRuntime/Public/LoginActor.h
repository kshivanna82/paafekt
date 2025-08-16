// LoginActor.h (fixed)
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LoginActor.generated.h"

class UAuthSubsystem;

UCLASS()
class INTELLISPACERUNTIME_API ALoginActor : public AActor
{
    GENERATED_BODY()
public:
    ALoginActor();

protected:
    virtual void BeginPlay() override;

    UFUNCTION() void OnOtpStarted(bool bOk);
    UFUNCTION() void OnOtpVerified(bool bOk);

    UFUNCTION(BlueprintCallable) void ProceedToMain();

private:
    UPROPERTY() UAuthSubsystem* Auth = nullptr;
};
