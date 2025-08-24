#pragma once
#include "GameFramework/Actor.h"
#include "LoginUIBoot.generated.h"

UCLASS()
class INTELLISPACERUNTIME_API ALoginUIBoot : public AActor
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, Category="Login")
    TSubclassOf<class ULoginWidget> LoginWidgetClass;

    // Map to open after login (set in details panel)
    UPROPERTY(EditAnywhere, Category="Login")
    FName MainMapName = "MainMap";

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY() class ULoginWidget* LoginWidget;
    FString PendingPhone;
    FString ServerGeneratedOtp; // demo only; in prod, do not store here

    UFUNCTION() void HandleSendOtp(const FString& Phone, int32 OtpDigits);
    UFUNCTION() void HandleVerifyOtp(const FString& Phone, const FString& OtpInput);

    void GoToMain();
};
