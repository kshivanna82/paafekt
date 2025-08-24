#include "LoginWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"

void ULoginWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (SendOtpButton)  SendOtpButton->OnClicked.AddDynamic(this, &ULoginWidget::HandleSendOtp);
    if (VerifyButton)   VerifyButton->OnClicked.AddDynamic(this, &ULoginWidget::HandleVerify);
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
