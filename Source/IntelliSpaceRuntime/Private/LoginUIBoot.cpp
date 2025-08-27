#include "LoginUIBoot.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/PanelWidget.h"
#include "Components/PanelSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/CanvasPanelSlot.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"                 // ✅ needed: APawn complete type
#include "Engine/World.h"

#include "SegDecorActor.h"

// Small helper: try to set padding on whatever slot the widget lives in.
static void TrySetSlotPadding(UWidget* Widget, const FMargin& Pad)
{
    if (!Widget || !Widget->Slot) return;

    if (auto* V = Cast<UVerticalBoxSlot>(Widget->Slot)) { V->SetPadding(Pad); return; }
    if (auto* H = Cast<UHorizontalBoxSlot>(Widget->Slot)) { H->SetPadding(Pad); return; }
    if (auto* C = Cast<UCanvasPanelSlot>(Widget->Slot))   { C->SetAutoSize(true); /* Canvas uses Offsets, no padding */ }
    // Add more slot types here if you use them (UniformGridSlot, GridSlot, etc.)
}

// Utility: make a simple colored brush for button states
static FSlateBrush MakeFlatBrush(const FLinearColor& Tint)
{
    FSlateBrush Brush;
    Brush.TintColor = FSlateColor(Tint);
    Brush.DrawAs = ESlateBrushDrawType::Box;
    Brush.Margin = FMargin(0.25f);
    return Brush;
}

void ULoginUIBoot::NativeConstruct()
{
    Super::NativeConstruct();

    StyleTextBoxes();
    StyleVerifyButton();

    if (VerifyButton)
    {
        VerifyButton->OnClicked.RemoveAll(this);
        VerifyButton->OnClicked.AddDynamic(this, &ULoginUIBoot::OnVerifyClicked);
    }

    // ✅ set layout padding via slot (not on the text box itself)
    TrySetSlotPadding(PhoneTextBox, FMargin(12.f, 8.f));
    TrySetSlotPadding(OtpTextBox,   FMargin(12.f, 8.f));
}

void ULoginUIBoot::StyleTextBoxes()
{
    if (PhoneTextBox)
    {
        PhoneTextBox->SetHintText(FText::FromString(TEXT("Enter phone number")));
        PhoneTextBox->SetIsPassword(false);
        PhoneTextBox->WidgetStyle.ForegroundColor = FSlateColor(FLinearColor::White);
        PhoneTextBox->WidgetStyle.BackgroundImageNormal  = MakeFlatBrush(FLinearColor(0.f,0.f,0.f,0.65f));
        PhoneTextBox->WidgetStyle.BackgroundImageHovered = MakeFlatBrush(FLinearColor(0.05f,0.05f,0.05f,0.75f));
        PhoneTextBox->WidgetStyle.BackgroundImageFocused = MakeFlatBrush(FLinearColor(0.0f,0.3f,0.0f,0.85f));
    }

    if (OtpTextBox)
    {
        OtpTextBox->SetHintText(FText::FromString(TEXT("Enter OTP")));
        OtpTextBox->SetIsPassword(false);
        OtpTextBox->WidgetStyle.ForegroundColor = FSlateColor(FLinearColor::White);
        OtpTextBox->WidgetStyle.BackgroundImageNormal  = MakeFlatBrush(FLinearColor(0.f,0.f,0.f,0.65f));
        OtpTextBox->WidgetStyle.BackgroundImageHovered = MakeFlatBrush(FLinearColor(0.05f,0.05f,0.05f,0.75f));
        OtpTextBox->WidgetStyle.BackgroundImageFocused = MakeFlatBrush(FLinearColor(0.0f,0.3f,0.0f,0.85f));
    }
}

void ULoginUIBoot::StyleVerifyButton()
{
    if (!VerifyButton) return;

    FButtonStyle Btn;
    Btn.SetNormal (MakeFlatBrush(FLinearColor(0.07f, 0.45f, 0.10f, 1.f)));
    Btn.SetHovered(MakeFlatBrush(FLinearColor(0.12f, 0.60f, 0.18f, 1.f)));
    Btn.SetPressed(MakeFlatBrush(FLinearColor(0.03f, 0.30f, 0.07f, 1.f)));
    Btn.SetDisabled(MakeFlatBrush(FLinearColor(0.2f, 0.2f, 0.2f, 0.6f)));
    Btn.SetNormalPadding (FMargin(8.f));
    Btn.SetPressedPadding(FMargin(10.f, 6.f, 6.f, 10.f));

    VerifyButton->SetStyle(Btn);
    VerifyButton->SetIsEnabled(true);

    if (VerifyLabel)
    {
        VerifyLabel->SetText(FText::FromString(TEXT("Verify")));
        VerifyLabel->SetJustification(ETextJustify::Center);
        VerifyLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        FSlateFontInfo FontInfo = VerifyLabel->GetFont();
        FontInfo.Size = 22;
        FontInfo.TypefaceFontName = FName("Bold");
        VerifyLabel->SetFont(FontInfo);
    }
}

void ULoginUIBoot::OnVerifyClicked()
{
    const bool bHasPhone = PhoneTextBox && !PhoneTextBox->GetText().IsEmpty();
    const bool bHasOtp   = OtpTextBox   && !OtpTextBox->GetText().IsEmpty();
    if (!bHasPhone || !bHasOtp)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ Phone/OTP missing."));
        return;
    }

    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (!PC) return;

    AActor* Found = UGameplayStatics::GetActorOfClass(World, ASegDecorActor::StaticClass());
    if (!Found)
    {
        UE_LOG(LogTemp, Error, TEXT("SegDecorActor not found in level."));
        return;
    }

    // ✅ now APawn is a complete type, so this Cast compiles
    if (APawn* AsPawn = Cast<APawn>(Found))
    {
        PC->Possess(AsPawn);
        UE_LOG(LogTemp, Log, TEXT("✅ Possessed SegDecorActor Pawn."));
    }
    else
    {
        PC->SetViewTargetWithBlend(Found, 0.5f);
        UE_LOG(LogTemp, Log, TEXT("✅ Switched view to SegDecorActor."));
    }
}
