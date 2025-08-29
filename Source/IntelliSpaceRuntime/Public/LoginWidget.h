#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoginWidget.generated.h"

class UEditableTextBox;
class UButton;
class UPanelWidget;
class UTextBlock;

UCLASS()
class INTELLISPACERUNTIME_API ULoginWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Delegates (use VALUE params to avoid AddDynamic signature mismatches)
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam  (FOnSendOtp,   FString, Phone);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVerifyOtp, FString, Phone, FString, Code);

    UPROPERTY(BlueprintAssignable) FOnSendOtp   OnSendOtp;
    UPROPERTY(BlueprintAssignable) FOnVerifyOtp OnVerifyOtp;

    UFUNCTION() void HandleSend();
    UFUNCTION() void HandleVerify();

protected:
    virtual void NativeOnInitialized() override;

    // Required widgets
    UPROPERTY(meta=(BindWidget)) UEditableTextBox* PhoneBox = nullptr;
    UPROPERTY(meta=(BindWidget)) UEditableTextBox* OtpBox   = nullptr;
    UPROPERTY(meta=(BindWidget)) UButton*          SendOtpButton = nullptr;
    UPROPERTY(meta=(BindWidget)) UButton*          VerifyButton  = nullptr;

    // Optional (so BP compiles even if these are absent)
    UPROPERTY(meta=(BindWidgetOptional)) UPanelWidget* OtpRow = nullptr;
    UPROPERTY(meta=(BindWidgetOptional)) UTextBlock*   SendOtpLabel = nullptr;
    UPROPERTY(meta=(BindWidgetOptional)) UTextBlock*   VerifyLabel  = nullptr;

private:
    bool IsDigitsOnly(const FString& S);
    void ShowOtpStep();
};
