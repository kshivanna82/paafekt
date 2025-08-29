#include "LoginUIBoot.h"
#include "LoginWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/UserWidget.h" 
#include "Components/EditableTextBox.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "SegDecorActor.h"   // ASegDecorActor

UPROPERTY(meta=(BindWidget)) UEditableTextBox* PhoneBox = nullptr;
UPROPERTY(meta=(BindWidget)) UEditableTextBox* OtpBox   = nullptr;

UPROPERTY(meta=(BindWidget)) UButton* SendOtpButton = nullptr;
UPROPERTY(meta=(BindWidget)) UButton* VerifyButton  = nullptr;

// NEW: labels inside the buttons
UPROPERTY(meta=(BindWidget)) UTextBlock* SendOtpLabel = nullptr;
UPROPERTY(meta=(BindWidget)) UTextBlock* VerifyLabel  = nullptr;

void ULoginUIBoot::Init(UWorld* World)
{
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("[LoginUIBoot] Init: World is null"));
        return;
    }
    if (!LoginWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("[LoginUIBoot] Init: LoginWidgetClass not set"));
        return;
    }
    if (LoginWidget && LoginWidget->IsInViewport())
    {
        UE_LOG(LogTemp, Verbose, TEXT("[LoginUIBoot] Init: Widget already shown"));
        return;
    }

    LoginWidget = CreateWidget<ULoginWidget>(World, LoginWidgetClass);
    if (!LoginWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("[LoginUIBoot] Init: Failed to create widget"));
        return;
    }

    // Bind C++ to widget dispatchers
    LoginWidget->OnVerifyOtp.AddDynamic(this, &ULoginUIBoot::OnVerifyClicked);
    LoginWidget->OnSendOtp.AddDynamic(this, &ULoginUIBoot::OnSendClicked);

    LoginWidget->AddToViewport(1000);

    // Focus UI
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
    {
        PC->bShowMouseCursor = true;

        FInputModeUIOnly Mode;
        Mode.SetWidgetToFocus(LoginWidget->TakeWidget());
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(Mode);
    }

    UE_LOG(LogTemp, Log, TEXT("[LoginUIBoot] Init: Widget shown and delegates bound"));
}
void ULoginUIBoot::NativeConstruct()
{
    Super::NativeConstruct();

    // Ensure the Verify button has a visible label.
    EnsureVerifyLabel();

    if (VerifyButton)
    {
        VerifyButton->OnClicked.AddDynamic(this, &ULoginUIBoot::OnVerifyClicked);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[LoginUIBoot] VerifyButton is null. BindWidgetOptional not found in designer."));
    }
}

void ULoginUIBoot::EnsureVerifyLabel()
{
    if (!VerifyButton)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LoginUIBoot] EnsureVerifyLabel skipped: VerifyButton is null."));
        return;
    }

    if (!VerifyLabel)
    {
        // Create a TextBlock and make it the content of the button so the label always shows.
        VerifyLabel = NewObject<UTextBlock>(VerifyButton, UTextBlock::StaticClass());
        VerifyLabel->SetText(FText::FromString(TEXT("Verify OTP")));
        VerifyLabel->SetJustification(ETextJustify::Center);

        // Style a bit
        FSlateFontInfo FontInfo = VerifyLabel->GetFont();
        FontInfo.Size = 22;
        VerifyLabel->SetFont(FontInfo);
        VerifyLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));

        // UButton is a UContentWidget → we can assign the content directly.
        VerifyButton->SetContent(VerifyLabel);
        UE_LOG(LogTemp, Log, TEXT("[LoginUIBoot] Injected VerifyLabel into VerifyButton."));
    }
    else
    {
        // Ensure correct caption in case designer text was blank.
        VerifyLabel->SetText(FText::FromString(TEXT("Verify OTP")));
    }
}

void ULoginUIBoot::OnSendClicked(FString Phone)
{
    UE_LOG(LogTemp, Log, TEXT("[LoginUIBoot] Send OTP: %s"), *Phone);
}

void ULoginUIBoot::OnVerifyClicked(FString Phone, FString Code)
{
    UE_LOG(LogTemp, Log, TEXT("[LoginUIBoot] Verify: Phone=%s Code=%s"), *Phone, *Code);
    NavigateToSegDecor();
}

void ULoginUIBoot::NavigateToSegDecor()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("[LoginUIBoot] No World in NavigateToSegDecor."));
        return;
    }

    // Remove login UI
    RemoveFromParent();

    // Restore game input
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        FInputModeGameOnly GameInput;
        PC->SetInputMode(GameInput);
        PC->bShowMouseCursor = false;
    }

    // Resolve target class (BP override if provided, otherwise hard-coded ASegDecorActor)
    UClass* TargetClass = SegDecorActorOverrideClass
        ? SegDecorActorOverrideClass.Get()
        : ASegDecorActor::StaticClass();


    if (!TargetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[LoginUIBoot] Target SegDecor class is null."));
        return;
    }

    // Find existing SegDecorActor
    AActor* Target = UGameplayStatics::GetActorOfClass(World, TargetClass);
    if (!Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LoginUIBoot] SegDecorActor not found; spawning one..."));
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        Target = World->SpawnActor<AActor>(TargetClass, SegDecorSpawnTransform, Params);
        if (!Target)
        {
            UE_LOG(LogTemp, Error, TEXT("[LoginUIBoot] Failed to spawn SegDecorActor."));
            return;
        }
    }

    // Switch view to the actor
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        PC->SetViewTargetWithBlend(Target, /*BlendTime=*/0.5f);
        UE_LOG(LogTemp, Log, TEXT("[LoginUIBoot] ✅ Switched view to SegDecorActor."));
    }
}
