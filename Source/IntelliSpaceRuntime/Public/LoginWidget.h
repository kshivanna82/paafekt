// Source/IntelliSpaceRuntime/Public/LoginWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h" 
#include "LoginWidget.generated.h"

class UEditableTextBox;
class UButton;
class UPanelWidget; // for grouping/hiding OTP row (e.g., HorizontalBox/SizeBox/Canvas slot)
class UTextBlock; 

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSendOtp, FString, Phone);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVerifyOtp, FString, Phone, FString, Code);

UCLASS()
class INTELLISPACERUNTIME_API ULoginWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable) FOnSendOtp OnSendOtp;
    UPROPERTY(BlueprintAssignable) FOnVerifyOtp OnVerifyOtp;

protected:
    virtual void NativeOnInitialized() override;

    UFUNCTION() void HandleSend();
    UFUNCTION() void HandleVerify();

public:
    // Bind these from WBP_Login
    UPROPERTY(meta=(BindWidget)) UEditableTextBox* PhoneBox = nullptr;
    UPROPERTY(meta=(BindWidget)) UButton*         SendOtpButton = nullptr;

    // OTP section (start hidden). If you don’t have a container, bind OtpBox+VerifyButton directly.
    UPROPERTY(meta=(BindWidgetOptional)) UPanelWidget* OtpRow = nullptr;
    UPROPERTY(meta=(BindWidget)) UEditableTextBox* OtpBox = nullptr;
    UPROPERTY(meta=(BindWidget)) UButton*         VerifyButton = nullptr;
    
    // NEW: labels inside the buttons
    UPROPERTY(meta=(BindWidgetOptional)) UTextBlock* SendOtpLabel = nullptr;
    UPROPERTY(meta=(BindWidgetOptional)) UTextBlock* VerifyLabel  = nullptr;

    // Call to switch UI from “send” step to “otp verify” step
    UFUNCTION(BlueprintCallable) void ShowOtpStep();

    // Helpers
    UFUNCTION(BlueprintCallable) static bool IsDigitsOnly(const FString& S);
};
