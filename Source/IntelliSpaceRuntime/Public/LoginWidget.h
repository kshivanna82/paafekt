#pragma once
#include "Blueprint/UserWidget.h"
#include "LoginWidget.generated.h"

class UEditableTextBox;
class UButton;
class UTextBlock;

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
protected:
    // Click handlers we bind in .cpp (must exist!)
    UFUNCTION() void OnSendOtpClicked();
    UFUNCTION() void OnVerifyClicked();

    // Helper used by .cpp to style and label buttons
    void ApplyButtonStyle(UButton* Btn, const FText& LabelText, UTextBlock*& OutLabel);

private:
    // Runtime-created text labels for the buttons
    UPROPERTY() UTextBlock* SendOtpLabel = nullptr;
    UPROPERTY() UTextBlock* VerifyLabel  = nullptr;
    
    UFUNCTION() void HandleSendOtp();
    UFUNCTION() void HandleVerify();
};
