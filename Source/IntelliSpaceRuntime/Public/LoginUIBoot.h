#pragma once
#include "GameFramework/Actor.h"
#include "LoginUIBoot.generated.h"

class UEditableTextBox;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSendOtp, const FString&, Phone, int32, Digits);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVerifyOtp, const FString&, Phone, const FString&, Code);

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
    
    // Set this to SegDecorActor (or BP_SegDecorActor) in the Editor
    UPROPERTY(EditDefaultsOnly, Category="Login")
    TSubclassOf<AActor> U2NetActorClass;

protected:
    virtual void BeginPlay() override;

private:
    // Runtime-created text labels for the buttons
    UPROPERTY() UTextBlock* SendOtpLabel = nullptr;
    UPROPERTY() UTextBlock* VerifyLabel  = nullptr;
    UPROPERTY() class ULoginWidget* LoginWidget;
    FString PendingPhone;
    FString ServerGeneratedOtp; // demo only; in prod, do not store here

    UFUNCTION() void HandleSendOtp(const FString& Phone, int32 OtpDigits);
    UFUNCTION() void HandleVerifyOtp(const FString& Phone, const FString& OtpInput);

//    void GoToMain();
    void EnterU2NetMode();
    void SwitchToGameInput();
};
