#include "LoginUIBoot.h"
#include "LoginWidget.h"               // ULoginWidget  delegates
#include "Blueprint/UserWidget.h"      // CreateWidget
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "SegDecorActor.h"

void ULoginUIBoot::Init(UWorld* World)
{
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("[LoginUIBoot] Init: World is null"));
        return;
    }
    if (!LoginWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[LoginUIBoot] Init: LoginWidgetClass not set"));
        return;
    }
        // Create with PlayerController as context (safer overload)
        APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
        if (!PC)
        {
            UE_LOG(LogTemp, Error, TEXT("[LoginUIBoot] Init: No PlayerController"));
            return;
        }
    
        if (LoginWidget && LoginWidget->IsInViewport())
        {
            UE_LOG(LogTemp, Verbose, TEXT("[LoginUIBoot] Init: Widget already visible"));
            return;
        }
    
        LoginWidget = CreateWidget<ULoginWidget>(PC, LoginWidgetClass);
    if (!LoginWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("[LoginUIBoot] Init: CreateWidget failed"));
        return;
    }

    // Bind to widget delegates (signatures MUST match exactly)
    LoginWidget->OnSendOtp  .AddDynamic(this, &ULoginUIBoot::OnSendClicked);
    LoginWidget->OnVerifyOtp.AddDynamic(this, &ULoginUIBoot::OnVerifyClicked);
     

    LoginWidget->AddToViewport(1000);

    // Focus UI
    PC->bShowMouseCursor = true;
    {
        FInputModeUIOnly Mode;
        Mode.SetWidgetToFocus(LoginWidget->TakeWidget());
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(Mode);
    }

    UE_LOG(LogTemp, Log, TEXT("[LoginUIBoot] Init: Widget shown  delegates bound"));
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
        UE_LOG(LogTemp, Error, TEXT("[LoginUIBoot] NavigateToSegDecor: No World"));
        return;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[LoginUIBoot] NavigateToSegDecor: No PlayerController"));
        return;
    }

    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsOfClass(World, ASegDecorActor::StaticClass(), Found);
    if (Found.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LoginUIBoot] No SegDecorActor in level"));
        return;
    }

    PC->SetViewTargetWithBlend(Found[0], 0.35f);

    // Return to game input and close the UI
    FInputModeGameOnly GameMode;
    PC->SetInputMode(GameMode);
    PC->bShowMouseCursor = false;

    if (LoginWidget)
    {
        LoginWidget->RemoveFromParent();
        LoginWidget = nullptr;
    }

    UE_LOG(LogTemp, Log, TEXT("[LoginUIBoot] Navigated to SegDecorActor and closed login UI"));
}
