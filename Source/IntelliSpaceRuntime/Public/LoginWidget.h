#pragma once
#include "Blueprint/UserWidget.h"
#include "LoginWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSendOtpRequested, const FString&, PhoneNumber, int32, OtpDigits);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVerifyOtpRequested, const FString&, PhoneNumber, const FString&, OtpInput);

UCLASS()
class INTELLISPACERUNTIME_API ULoginWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(meta=(BindWidget)) class UEditableTextBox* PhoneBox;
    UPROPERTY(meta=(BindWidget)) class UEditableTextBox* OtpBox;
    UPROPERTY(meta=(BindWidget)) class UButton* SendOtpButton;
    UPROPERTY(meta=(BindWidget)) class UButton* VerifyButton;

    UPROPERTY(BlueprintAssignable, Category="Login")
    FOnSendOtpRequested OnSendOtpRequested;

    UPROPERTY(BlueprintAssignable, Category="Login")
    FOnVerifyOtpRequested OnVerifyOtpRequested;

    virtual void NativeOnInitialized() override;

private:
    UFUNCTION() void HandleSendOtp();
    UFUNCTION() void HandleVerify();
};
