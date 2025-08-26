#include "LoginWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"
#include "Fonts/SlateFontInfo.h"

// Small helper for flat button brushes
static FSlateBrush MakeFlatBrush(const FLinearColor& Tint)
{
    FSlateBrush B;
    B.DrawAs    = ESlateBrushDrawType::Box;
    B.TintColor = Tint;
    return B;
}

void ULoginWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    const FSlateColor TextCol(FLinearColor(0.f, 0.f, 0.f));
    const FSlateColor HintCol(FLinearColor(0.6f, 0.6f, 0.6f, 1.f));
    const FLinearColor BgCol(1.f, 1.f, 1.f, 1.f);

    if (PhoneBox)
    {
        PhoneBox->SetHintText(FText::FromString(TEXT("Enter phone number")));

        // Make typed text dark & larger (works on all versions)
        PhoneBox->WidgetStyle.TextStyle.ColorAndOpacity = FSlateColor(FLinearColor::Black);
        PhoneBox->WidgetStyle.SetForegroundColor(FSlateColor(FLinearColor::Black));
        PhoneBox->WidgetStyle.TextStyle.Font.Size = 22;   // pick your size
    }
    if (OtpBox)
    {
        OtpBox->SetHintText(FText::FromString(TEXT("Enter OTP")));
        
        OtpBox->WidgetStyle.TextStyle.ColorAndOpacity = FSlateColor(FLinearColor::Black);
        OtpBox->WidgetStyle.SetForegroundColor(FSlateColor(FLinearColor::Black));
        OtpBox->WidgetStyle.TextStyle.Font.Size = 22;
    }
    // 2) Buttons: flat style + labels
    const FLinearColor Normal  (0.16f, 0.16f, 0.16f, 1.f); // dark gray
    const FLinearColor Hovered (0.22f, 0.22f, 0.22f, 1.f); // a bit lighter on hover
    const FLinearColor Pressed (0.10f, 0.10f, 0.10f, 1.f); // deeper on press

    const FMargin ContentPad(18.f, 12.f); // comfy padding
    const FSlateFontInfo FontInfo = FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 28);

    auto ApplyButtonStyle = [&](UButton* Btn, const TCHAR* LabelText, UTextBlock*& OutLabel)
    {
        if (!Btn) return;

        // Style
        FButtonStyle Style;
        Style.SetNormal ( MakeFlatBrush(Normal)  );
        Style.SetHovered( MakeFlatBrush(Hovered) );
        Style.SetPressed( MakeFlatBrush(Pressed) );
        Style.SetDisabled(MakeFlatBrush(FLinearColor(0.16f,0.16f,0.16f,0.5f)));
        Style.SetNormalPadding(ContentPad);
        Style.SetPressedPadding(FMargin(ContentPad.Left, ContentPad.Top + 1.f, ContentPad.Right, ContentPad.Bottom - 1.f));
        Btn->SetStyle(Style);

        // Label (TextBlock lives as child content of the button)
        if (!OutLabel)
        {
            OutLabel = NewObject<UTextBlock>(Btn);
            OutLabel->SetText(FText::FromString(LabelText));
            OutLabel->SetFont(FontInfo);
            OutLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
            Btn->SetContent(OutLabel);
        }
        else
        {
            OutLabel->SetText(FText::FromString(LabelText));
            OutLabel->SetFont(FontInfo);
            OutLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        }
    };

    ApplyButtonStyle(SendOtpButton, TEXT("Send OTP"), SendOtpLabel);
    ApplyButtonStyle(VerifyButton,  TEXT("Verify & Continue"), VerifyLabel);

    // 3) Click bindings
    if (SendOtpButton)
    {
        SendOtpButton->OnClicked.Clear();
        SendOtpButton->OnClicked.AddDynamic(this, &ULoginWidget::OnSendOtpClicked);
    }
    if (VerifyButton)
    {
        VerifyButton->OnClicked.Clear();
        VerifyButton->OnClicked.AddDynamic(this, &ULoginWidget::OnVerifyClicked);
    }
}

// === Button handlers ===
void ULoginWidget::OnSendOtpClicked()
{
    const FString Phone = PhoneBox ? PhoneBox->GetText().ToString() : TEXT("");
    const int32 OtpDigits = 6; // could also be a UPROPERTY if you want to expose
    OnSendOtpRequested.Broadcast(Phone, OtpDigits);
}

void ULoginWidget::OnVerifyClicked()
{
    const FString Phone = PhoneBox ? PhoneBox->GetText().ToString() : TEXT("");
    const FString Code  = OtpBox   ? OtpBox->GetText().ToString()   : TEXT("");
    OnVerifyOtpRequested.Broadcast(Phone, Code);
}

void ULoginWidget::HandleSendOtp()
{
    const FString Phone = PhoneBox ? PhoneBox->GetText().ToString() : FString();
    const int32 OtpDigits = 6; // typical
    OnSendOtpRequested.Broadcast(Phone, OtpDigits);
}

void ULoginWidget::HandleVerify()
{
    const FString Phone = PhoneBox ? PhoneBox->GetText().ToString() : FString();
    const FString Otp   = OtpBox   ? OtpBox->GetText().ToString()   : FString();
    OnVerifyOtpRequested.Broadcast(Phone, Otp);
}
