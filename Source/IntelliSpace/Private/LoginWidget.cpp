// LoginWidget.cpp
#include "LoginWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Border.h"
#include "Components/Spacer.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/Texture2D.h"
#include "Http.h"
#include "Json.h"
#include "JsonObjectConverter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Styling/SlateTypes.h"

ULoginWidget::ULoginWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    UE_LOG(LogTemp, Warning, TEXT("LoginWidget: Constructor called"));
    bIsOtpSent = false;
    OtpExpiryTime = 300.0f;
}

void ULoginWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Warning, TEXT("=== LoginWidget: NativeConstruct Started ==="));
    
    // Check widget bindings
    UE_LOG(LogTemp, Warning, TEXT("LoginWidget: PhoneBox = %s"), PhoneBox ? TEXT("BOUND") : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("LoginWidget: OtpBox = %s"), OtpBox ? TEXT("BOUND") : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("LoginWidget: SendOtpButton = %s"), SendOtpButton ? TEXT("BOUND") : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("LoginWidget: VerifyButton = %s"), VerifyButton ? TEXT("BOUND") : TEXT("NULL"));
    
    SetupUIElements();
    
    if (SendOtpButton)
    {
        SendOtpButton->OnClicked.AddDynamic(this, &ULoginWidget::OnSendOtpClicked);
        UE_LOG(LogTemp, Warning, TEXT("LoginWidget: SendOtpButton click event bound"));
    }
    
    if (VerifyButton)
    {
        VerifyButton->OnClicked.AddDynamic(this, &ULoginWidget::OnVerifyClicked);
        VerifyButton->SetIsEnabled(false);
        UE_LOG(LogTemp, Warning, TEXT("LoginWidget: VerifyButton click event bound, initially disabled"));
    }
    
    if (PhoneBox)
    {
        PhoneBox->SetKeyboardFocus();
        UE_LOG(LogTemp, Warning, TEXT("LoginWidget: Focus set to PhoneBox"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("=== LoginWidget: NativeConstruct Complete ==="));
}

void ULoginWidget::SetupUIElements()
{
    UE_LOG(LogTemp, Warning, TEXT("LoginWidget: SetupUIElements started"));
    
    // Setup PhoneBox
    if (PhoneBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("LoginWidget: Configuring PhoneBox"));
        
        // Set hint text - THIS FIXES THE WHITE RECTANGLE
        PhoneBox->SetHintText(FText::FromString("Enter 10-digit phone number"));
        
        // Verify hint was set
        FText HintText = PhoneBox->GetHintText();
        UE_LOG(LogTemp, Warning, TEXT("LoginWidget: PhoneBox hint text = '%s'"), *HintText.ToString());
        
        // Set colors for visibility
        PhoneBox->SetForegroundColor(FLinearColor::Black);
        
        // Try to modify the style for background
        FEditableTextBoxStyle NewStyle = PhoneBox->WidgetStyle;
        
        // Create a light gray background
        FSlateBrush BackgroundBrush;
        BackgroundBrush.TintColor = FSlateColor(FLinearColor(0.95f, 0.95f, 0.95f, 1.0f));
        BackgroundBrush.DrawAs = ESlateBrushDrawType::Box;
        BackgroundBrush.Margin = FMargin(4.0f);
        
        NewStyle.BackgroundImageNormal = BackgroundBrush;
        NewStyle.BackgroundImageHovered = BackgroundBrush;
        NewStyle.BackgroundImageFocused = BackgroundBrush;
        NewStyle.BackgroundImageReadOnly = BackgroundBrush;
        
        // Set the style
        PhoneBox->WidgetStyle = NewStyle;
        
        PhoneBox->SetIsReadOnly(false);
        PhoneBox->SetIsPassword(false);
        PhoneBox->OnTextChanged.AddDynamic(this, &ULoginWidget::OnPhoneTextChanged);
        
        UE_LOG(LogTemp, Warning, TEXT("LoginWidget: PhoneBox setup complete"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("LoginWidget: PhoneBox is NULL - CHECK BLUEPRINT BINDING"));
    }
    
    // Setup OtpBox
    if (OtpBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("LoginWidget: Configuring OtpBox"));
        
        // INITIALLY HIDE THE OTP BOX
        OtpBox->SetVisibility(ESlateVisibility::Collapsed);
        
        // Set hint text - THIS FIXES THE WHITE RECTANGLE
        OtpBox->SetHintText(FText::FromString("Enter 6-digit OTP"));
        
        // Verify hint was set
        FText HintText = OtpBox->GetHintText();
        UE_LOG(LogTemp, Warning, TEXT("LoginWidget: OtpBox hint text = '%s'"), *HintText.ToString());
        
        // Set colors for visibility
        OtpBox->SetForegroundColor(FLinearColor::Black);
        
        // Try to modify the style for background
        FEditableTextBoxStyle NewStyle = OtpBox->WidgetStyle;
        
        // Create a light gray background
        FSlateBrush BackgroundBrush;
        BackgroundBrush.TintColor = FSlateColor(FLinearColor(0.95f, 0.95f, 0.95f, 1.0f));
        BackgroundBrush.DrawAs = ESlateBrushDrawType::Box;
        BackgroundBrush.Margin = FMargin(4.0f);
        
        NewStyle.BackgroundImageNormal = BackgroundBrush;
        NewStyle.BackgroundImageHovered = BackgroundBrush;
        NewStyle.BackgroundImageFocused = BackgroundBrush;
        NewStyle.BackgroundImageReadOnly = BackgroundBrush;
        
        // Set the style
        OtpBox->WidgetStyle = NewStyle;
        
        OtpBox->SetIsPassword(false);  // Changed to false so user can see the OTP they enter
        OtpBox->SetIsReadOnly(false);
        OtpBox->OnTextChanged.AddDynamic(this, &ULoginWidget::OnOtpTextChanged);
        
        UE_LOG(LogTemp, Warning, TEXT("LoginWidget: OtpBox setup complete"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("LoginWidget: OtpBox is NULL - CHECK BLUEPRINT BINDING"));
    }
    
    // Setup labels if they exist
    if (PhoneLabel)
    {
        PhoneLabel->SetText(FText::FromString("Phone Number:"));
        PhoneLabel->SetColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f));
        UE_LOG(LogTemp, Warning, TEXT("LoginWidget: PhoneLabel configured"));
    }
    
    if (OtpLabel)
    {
        OtpLabel->SetText(FText::FromString("OTP Code:"));
        OtpLabel->SetColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f));
        OtpLabel->SetVisibility(ESlateVisibility::Collapsed); // Initially hidden
        UE_LOG(LogTemp, Warning, TEXT("LoginWidget: OtpLabel configured"));
    }
    
    // Setup buttons
    if (SendOtpButton)
    {
        if (UTextBlock* ButtonText = Cast<UTextBlock>(SendOtpButton->GetChildAt(0)))
        {
            ButtonText->SetText(FText::FromString("SEND OTP"));
            ButtonText->SetColorAndOpacity(FLinearColor::White);
        }
        FButtonStyle ButtonStyle = SendOtpButton->GetStyle();
        ButtonStyle.Normal.TintColor = FLinearColor(0.0f, 0.52f, 1.0f, 1.0f);
        ButtonStyle.Hovered.TintColor = FLinearColor(0.3f, 0.64f, 1.0f, 1.0f);
        ButtonStyle.Pressed.TintColor = FLinearColor(0.0f, 0.4f, 0.8f, 1.0f);
        SendOtpButton->SetStyle(ButtonStyle);
        UE_LOG(LogTemp, Warning, TEXT("LoginWidget: SendOtpButton configured"));
    }
    
    if (VerifyButton)
    {
        // INITIALLY HIDE THE VERIFY BUTTON
        VerifyButton->SetVisibility(ESlateVisibility::Collapsed);
        
        if (UTextBlock* ButtonText = Cast<UTextBlock>(VerifyButton->GetChildAt(0)))
        {
            ButtonText->SetText(FText::FromString("VERIFY & LOGIN"));
            ButtonText->SetColorAndOpacity(FLinearColor::White);
        }
        FButtonStyle ButtonStyle = VerifyButton->GetStyle();
        ButtonStyle.Normal.TintColor = FLinearColor(0.16f, 0.65f, 0.27f, 1.0f);
        ButtonStyle.Hovered.TintColor = FLinearColor(0.36f, 0.75f, 0.46f, 1.0f);
        ButtonStyle.Pressed.TintColor = FLinearColor(0.12f, 0.49f, 0.20f, 1.0f);
        ButtonStyle.Disabled.TintColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.5f);
        VerifyButton->SetStyle(ButtonStyle);
        UE_LOG(LogTemp, Warning, TEXT("LoginWidget: VerifyButton configured"));
    }
    
    if (TitleText)
    {
        TitleText->SetText(FText::FromString("INTELLISPACE LOGIN"));
        FSlateFontInfo TitleFont = TitleText->GetFont();
        TitleFont.Size = 28;
        TitleText->SetFont(TitleFont);
        TitleText->SetColorAndOpacity(FLinearColor::White);
        UE_LOG(LogTemp, Warning, TEXT("LoginWidget: TitleText configured"));
    }
    
    if (StatusText)
    {
        StatusText->SetText(FText::GetEmpty());
        StatusText->SetColorAndOpacity(FLinearColor::Yellow);
        UE_LOG(LogTemp, Warning, TEXT("LoginWidget: StatusText configured"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("LoginWidget: SetupUIElements complete"));
}

void ULoginWidget::OnPhoneTextChanged(const FText& Text)
{
    FString PhoneNumber = Text.ToString();
    
    FString CleanNumber;
    for (TCHAR Char : PhoneNumber)
    {
        if (FChar::IsDigit(Char))
        {
            CleanNumber.AppendChar(Char);
        }
    }
    
    if (CleanNumber.Len() > 10)
    {
        CleanNumber = CleanNumber.Left(10);
        PhoneBox->SetText(FText::FromString(CleanNumber));
    }
    else if (CleanNumber != PhoneNumber)
    {
        PhoneBox->SetText(FText::FromString(CleanNumber));
    }
    
    if (SendOtpButton)
    {
        SendOtpButton->SetIsEnabled(CleanNumber.Len() == 10);
    }
}

void ULoginWidget::OnOtpTextChanged(const FText& Text)
{
    FString OtpCode = Text.ToString();
    
    FString CleanOtp;
    for (TCHAR Char : OtpCode)
    {
        if (FChar::IsDigit(Char))
        {
            CleanOtp.AppendChar(Char);
        }
    }
    
    if (CleanOtp.Len() > 6)
    {
        CleanOtp = CleanOtp.Left(6);
        OtpBox->SetText(FText::FromString(CleanOtp));
    }
    else if (CleanOtp != OtpCode)
    {
        OtpBox->SetText(FText::FromString(CleanOtp));
    }
    
    if (CleanOtp.Len() == 6 && bIsOtpSent)
    {
        OnVerifyClicked();
    }
}

void ULoginWidget::OnSendOtpClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("LoginWidget: Send OTP clicked"));
    
    if (!PhoneBox) return;
    
    FString PhoneNumber = PhoneBox->GetText().ToString();
    
    if (!ValidatePhoneNumber(PhoneNumber))
    {
        ShowMessage("Please enter a valid 10-digit phone number", FLinearColor::Red);
        return;
    }
    
    ShowMessage("Sending OTP...", FLinearColor::Yellow);
    SendOtpButton->SetIsEnabled(false);
    CurrentPhoneNumber = PhoneNumber;
    
    // FOR TESTING: Simulate successful OTP send
    FTimerHandle TestTimer;
    GetWorld()->GetTimerManager().SetTimer(TestTimer, [this]()
    {
        OnOtpSent(nullptr, nullptr, true);
    }, 1.0f, false);
    
    // Real implementation would call:
    // SendOtpRequest(PhoneNumber);
}

void ULoginWidget::OnVerifyClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("LoginWidget: Verify clicked"));
    
    if (!PhoneBox || !OtpBox) return;
    
    FString PhoneNumber = PhoneBox->GetText().ToString();
    FString OtpCode = OtpBox->GetText().ToString();
    
    if (OtpCode.Len() != 6)
    {
        ShowMessage("Please enter a valid 6-digit OTP", FLinearColor::Red);
        return;
    }
    
    ShowMessage("Verifying OTP...", FLinearColor::Yellow);
    VerifyButton->SetIsEnabled(false);
    
    // FOR TESTING: Accept any 6-digit code
    FTimerHandle TestTimer;
    GetWorld()->GetTimerManager().SetTimer(TestTimer, [this]()
    {
        OnOtpVerified(nullptr, nullptr, true);
    }, 1.0f, false);
    
    // Real implementation would call:
    // VerifyOtpRequest(PhoneNumber, OtpCode);
}

bool ULoginWidget::ValidatePhoneNumber(const FString& PhoneNumber)
{
    if (PhoneNumber.Len() != 10) return false;
    
    for (TCHAR Char : PhoneNumber)
    {
        if (!FChar::IsDigit(Char)) return false;
    }
    
    TCHAR FirstDigit = PhoneNumber[0];
    if (FirstDigit == '0' || FirstDigit == '1') return false;
    
    return true;
}

void ULoginWidget::SendOtpRequest(const FString& PhoneNumber)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindUObject(this, &ULoginWidget::OnOtpSent);
    Request->SetURL("https://api.intellispace.com/auth/send-otp");
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField("phone", PhoneNumber);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    Request->SetContentAsString(OutputString);
    Request->ProcessRequest();
}

void ULoginWidget::VerifyOtpRequest(const FString& PhoneNumber, const FString& OtpCode)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindUObject(this, &ULoginWidget::OnOtpVerified);
    Request->SetURL("https://api.intellispace.com/auth/verify-otp");
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField("phone", PhoneNumber);
    JsonObject->SetStringField("otp", OtpCode);
    
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    Request->SetContentAsString(OutputString);
    Request->ProcessRequest();
}

void ULoginWidget::OnOtpSent(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    // FOR TESTING: Always succeed
    bWasSuccessful = true;
    
    if (!bWasSuccessful)
    {
        ShowMessage("Failed to send OTP. Please try again.", FLinearColor::Red);
        SendOtpButton->SetIsEnabled(true);
        return;
    }
    
    bIsOtpSent = true;
    ShowMessage("OTP sent! (Testing: Enter any 6 digits)", FLinearColor::Green);
    
    if (VerifyButton) VerifyButton->SetIsEnabled(true);
    if (OtpBox)
    {
        OtpBox->SetIsEnabled(true);
        OtpBox->SetKeyboardFocus();
    }
    
    GetWorld()->GetTimerManager().SetTimer(OtpExpiryTimer, this, &ULoginWidget::OnOtpExpired, OtpExpiryTime, false);
    
    if (UTextBlock* ButtonText = Cast<UTextBlock>(SendOtpButton->GetChildAt(0)))
    {
        ButtonText->SetText(FText::FromString("RESEND OTP"));
    }
    SendOtpButton->SetIsEnabled(true);
    
    ShowOtpStep();
}

void ULoginWidget::OnOtpVerified(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    // FOR TESTING: Always succeed
    bWasSuccessful = true;
    
    if (!bWasSuccessful)
    {
        ShowMessage("Failed to verify OTP. Please try again.", FLinearColor::Red);
        VerifyButton->SetIsEnabled(true);
        return;
    }
    
    ShowMessage("Login successful! Welcome to IntelliSpace!", FLinearColor::Green);
    OnLoginSuccess("test_token");
    
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
    {
        UGameplayStatics::OpenLevel(GetWorld(), "MainMenu");
    }, 1.5f, false);
}

void ULoginWidget::OnOtpExpired()
{
    bIsOtpSent = false;
    ShowMessage("OTP has expired. Please request a new one.", FLinearColor(1.0f, 0.5f, 0.0f, 1.0f));
    
    if (VerifyButton) VerifyButton->SetIsEnabled(false);
    if (OtpBox) OtpBox->SetText(FText::GetEmpty());
}

void ULoginWidget::ShowMessage(const FString& Message, const FLinearColor& Color)
{
    UE_LOG(LogTemp, Warning, TEXT("LoginWidget: Message - %s"), *Message);
    
    if (StatusText)
    {
        StatusText->SetText(FText::FromString(Message));
        StatusText->SetColorAndOpacity(Color);
    }
}

void ULoginWidget::StoreAuthToken(const FString& Token)
{
    CurrentSessionToken = Token;
}

void ULoginWidget::ResetLoginForm()
{
    if (PhoneBox) PhoneBox->SetText(FText::GetEmpty());
    if (OtpBox) OtpBox->SetText(FText::GetEmpty());
    
    if (SendOtpButton)
    {
        SendOtpButton->SetIsEnabled(true);
        if (UTextBlock* ButtonText = Cast<UTextBlock>(SendOtpButton->GetChildAt(0)))
        {
            ButtonText->SetText(FText::FromString("SEND OTP"));
        }
    }
    
    if (VerifyButton) VerifyButton->SetIsEnabled(false);
    if (StatusText) StatusText->SetText(FText::GetEmpty());
    
    bIsOtpSent = false;
    GetWorld()->GetTimerManager().ClearTimer(OtpExpiryTimer);
    
    if (PhoneBox) PhoneBox->SetKeyboardFocus();
}

void ULoginWidget::ShowOtpStep()
{
    UE_LOG(LogTemp, Warning, TEXT("LoginWidget: ShowOtpStep called"));
    
    if (OtpBox)
    {
        OtpBox->SetVisibility(ESlateVisibility::Visible);
        OtpBox->SetKeyboardFocus();
    }
    
    if (OtpLabel)
    {
        OtpLabel->SetVisibility(ESlateVisibility::Visible);
    }
}
