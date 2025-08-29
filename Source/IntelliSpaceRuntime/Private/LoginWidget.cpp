login#include "LoginWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
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

    // Typed text styling
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

    // Button labels (optional children)
    if (SendOtpLabel)  SendOtpLabel->SetText(FText::FromString(TEXT("Send OTP")));
    if (VerifyLabel)   VerifyLabel->SetText(FText::FromString(TEXT("Verify")));

    // Bind clicks
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
    if (PhoneBox) PhoneBox->SetIsEnabled(false);
    if (SendOtpButton) SendOtpButton->SetVisibility(ESlateVisibility::Collapsed);

    // IMPORTANT: interactive controls must be Visible (not SelfHitTestInvisible)
    if (OtpRow)
    {
        OtpRow->SetVisibility(ESlateVisibility::Visible);
    }
    else
    {
        if (OtpBox)       OtpBox->SetVisibility(ESlateVisibility::Visible);
        if (VerifyButton) VerifyButton->SetVisibility(ESlateVisibility::Visible);
    }
}

void ULoginWidget::HandleSend()
{
    const FString Phone = PhoneBox ? PhoneBox->GetText().ToString() : TEXT("");
    if (!IsDigitsOnly(Phone)) return;

    ShowOtpStep();
    OnSendOtp.Broadcast(Phone); // VALUE param
}

void ULoginWidget::HandleVerify()
{
    const FString Phone = PhoneBox ? PhoneBox->GetText().ToString() : TEXT("");
    const FString Code  = OtpBox   ? OtpBox->GetText().ToString()   : TEXT("");
    OnVerifyOtp.Broadcast(Phone, Code); // VALUE params
}
