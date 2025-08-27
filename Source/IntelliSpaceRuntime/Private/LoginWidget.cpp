// Source/IntelliSpaceRuntime/Private/LoginWidget.cpp
#include "LoginWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateColor.h"
#include "Styling/SlateBrush.h"

bool ULoginWidget::IsDigitsOnly(const FString& S)
{
    for (TCHAR C : S)
    {
        if (!FChar::IsDigit(C)) return false;
    }
    return S.Len() > 0;
}

void ULoginWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // Make typed text dark and a bit larger (engine-agnostic)
    if (PhoneBox)
    {
        PhoneBox->SetHintText(FText::FromString(TEXT("Enter phone number")));
        PhoneBox->WidgetStyle.TextStyle.ColorAndOpacity = FSlateColor(FLinearColor::Black);
        PhoneBox->WidgetStyle.SetForegroundColor(FSlateColor(FLinearColor::Black));
        PhoneBox->WidgetStyle.TextStyle.Font.Size = 22;
    }
    if (OtpBox)
    {
        OtpBox->SetHintText(FText::FromString(TEXT("Enter OTP")));
        OtpBox->WidgetStyle.TextStyle.ColorAndOpacity = FSlateColor(FLinearColor::Black);
        OtpBox->WidgetStyle.SetForegroundColor(FSlateColor(FLinearColor::Black));
        OtpBox->WidgetStyle.TextStyle.Font.Size = 22;
    }

    // Start with OTP UI hidden
    if (OtpRow)
    {
        OtpRow->SetVisibility(ESlateVisibility::Collapsed);
    }
    else if (OtpBox && VerifyButton)
    {
        OtpBox->SetVisibility(ESlateVisibility::Collapsed);
        VerifyButton->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (SendOtpButton)
    {
        SendOtpButton->OnClicked.Clear();
        SendOtpButton->OnClicked.AddDynamic(this, &ULoginWidget::HandleSend);
    }
    if (VerifyButton)
    {
        VerifyButton->OnClicked.Clear();
        VerifyButton->OnClicked.AddDynamic(this, &ULoginWidget::HandleVerify);
    }
}

void ULoginWidget::ShowOtpStep()
{
    // Disable phone editing during OTP step
    if (PhoneBox)
    {
        PhoneBox->SetIsEnabled(false);
    }
    // Hide Send
    if (SendOtpButton)
    {
        SendOtpButton->SetVisibility(ESlateVisibility::Collapsed);
    }
    // Show OTP controls
    if (OtpRow)
    {
        OtpRow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }
    else
    {
        if (OtpBox)      OtpBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        if (VerifyButton)VerifyButton->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }
}

void ULoginWidget::HandleSend()
{
    const FString Phone = PhoneBox ? PhoneBox->GetText().ToString() : TEXT("");
    if (!IsDigitsOnly(Phone))
    {
        // Optional: add a simple red border, or just early-out.
        return;
    }

    // Switch UI to OTP step right away
    ShowOtpStep();

    // Notify code-behind
    OnSendOtp.Broadcast(Phone);
}

void ULoginWidget::HandleVerify()
{
    const FString Phone = PhoneBox ? PhoneBox->GetText().ToString() : TEXT("");
    const FString Code  = OtpBox   ? OtpBox->GetText().ToString()   : TEXT("");

    OnVerifyOtp.Broadcast(Phone, Code);
}
