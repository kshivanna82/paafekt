// LoginActor.cpp
#include "LoginActor.h"
#include "AuthSubsystem.h"
#include "LoginWidget.h"
#include "FurniLife.h"
#include "Engine/GameInstance.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

ALoginActor::ALoginActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ALoginActor::BeginPlay()
{
    Super::BeginPlay();
    
    // Get auth subsystem
    if (UGameInstance* GI = GetGameInstance())
    {
        Auth = GI->GetSubsystem<UAuthSubsystem>();
    }
    
    // ALWAYS SHOW LOGIN UI FOR TESTING
    UE_LOG(LogTemp, Warning, TEXT("[Login] Forcing login UI to show (Auth exists: %s)"), Auth ? TEXT("YES") : TEXT("NO"));
    ShowLoginUI();
    
    // Bind auth events if Auth exists
    if (Auth)
    {
        Auth->OnOtpStarted.AddDynamic(this, &ALoginActor::OnOtpStarted);
        Auth->OnOtpVerified.AddDynamic(this, &ALoginActor::OnOtpVerified);
    }
}

void ALoginActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    HideLoginUI();
    
    if (Auth)
    {
        Auth->OnOtpStarted.RemoveDynamic(this, &ALoginActor::OnOtpStarted);
        Auth->OnOtpVerified.RemoveDynamic(this, &ALoginActor::OnOtpVerified);
    }
    
    Super::EndPlay(EndPlayReason);
}

void ALoginActor::ShowLoginUI()
{
    if (!LoginWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Login] No LoginWidgetClass set - skipping to FurniLife"));
        SpawnFurniLife();
        return;
    }
    
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[Login] No PlayerController"));
        return;
    }
    
    // Create widget
    LoginWidget = CreateWidget<ULoginWidget>(PC, LoginWidgetClass);
    if (!LoginWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("[Login] Failed to create login widget"));
        SpawnFurniLife();
        return;
    }
    
    // Show widget
    LoginWidget->AddToViewport(1000);
    
    // Set input mode for UI
    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(LoginWidget->TakeWidget());
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PC->SetInputMode(InputMode);
    PC->bShowMouseCursor = true;
    
    UE_LOG(LogTemp, Log, TEXT("[Login] Login UI shown"));
}

void ALoginActor::HideLoginUI()
{
    if (LoginWidget)
    {
        LoginWidget->RemoveFromParent();
        LoginWidget = nullptr;
    }
    
    // Return to game input
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        FInputModeGameOnly GameMode;
        PC->SetInputMode(GameMode);
        PC->bShowMouseCursor = false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("[Login] Login UI hidden"));
}

void ALoginActor::SpawnFurniLife()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("[Login] No world to spawn FurniLife"));
        return;
    }
    
    // Hide login UI first
    HideLoginUI();
    
    // Check if FurniLife already exists
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AFurniLife::StaticClass(), FoundActors);
    
    if (FoundActors.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[Login] FurniLife already exists"));
    }
    else
    {
        // Spawn FurniLife actor
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        
        AFurniLife* FurniLifeActor = World->SpawnActor<AFurniLife>(
            AFurniLife::StaticClass(),
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            SpawnParams
        );
        
        if (FurniLifeActor)
        {
            UE_LOG(LogTemp, Log, TEXT("[Login] FurniLife spawned successfully"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[Login] Failed to spawn FurniLife"));
        }
    }
    
    // Broadcast completion
    OnLoginComplete.Broadcast();
    
    // Destroy this login actor
    Destroy();
}

void ALoginActor::OnSendOtpClicked(FString Phone)
{
    CurrentPhone = Phone;
    UE_LOG(LogTemp, Log, TEXT("[Login] Send OTP clicked for phone: %s"), *Phone);
    
    if (Auth)
    {
        Auth->StartOtp(Phone, TEXT("+1")); // Default country code
    }
}

void ALoginActor::OnVerifyOtpClicked(FString Phone, FString Code)
{
    UE_LOG(LogTemp, Log, TEXT("[Login] Verify OTP clicked - Code: %s"), *Code);
    
    if (Auth)
    {
        Auth->VerifyOtp(Code);
    }
}

void ALoginActor::OnOtpStarted(bool bSuccess)
{
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("[Login] OTP sent successfully"));
        
        // Show OTP input step in widget
        if (LoginWidget)
        {
            LoginWidget->ShowOtpStep();
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Login] Failed to send OTP"));
    }
}

void ALoginActor::OnOtpVerified(bool bSuccess)
{
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("[Login] OTP verified successfully - transitioning to FurniLife"));
        
        // Go to FurniLife
        SpawnFurniLife();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Login] Invalid OTP"));
        // Stay on login screen for retry
    }
}
