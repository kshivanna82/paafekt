// LoginActor.cpp
#include "LoginActor.h"
#include "AuthSubsystem.h"
#include "LoginWidget.h"
#include "FurniLife.h"
#include "Components/EditableTextBox.h"
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
    
    if (!Auth)
    {
        UE_LOG(LogTemp, Error, TEXT("[Login] AuthSubsystem missing"));
        // Skip login and go straight to FurniLife
        SpawnFurniLife();
        return;
    }
    
    // Bind auth events
    Auth->OnOtpStarted.AddDynamic(this, &ALoginActor::OnOtpStarted);
    Auth->OnOtpVerified.AddDynamic(this, &ALoginActor::OnOtpVerified);
    
    // FORCE SHOW LOGIN FOR TESTING - Comment this section back in when auth is working
    /*
    if (Auth->IsLoggedIn())
    {
        UE_LOG(LogTemp, Log, TEXT("[Login] Already logged in - going to FurniLife"));
        SpawnFurniLife();
    }
    else
    {
        ShowLoginUI();
    }
    */
    
    // ALWAYS SHOW LOGIN FOR NOW
    UE_LOG(LogTemp, Warning, TEXT("[Login] Forcing login UI to show for testing"));
    ShowLoginUI();
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
    if (!IsInGameThread())
    {
        AsyncTask(ENamedThreads::GameThread, [this]()
        {
            ShowLoginUI();
        });
        return;
    }
    
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;
    
    if (!LoginWidget && LoginWidgetClass)
    {
        LoginWidget = CreateWidget<ULoginWidget>(PC, LoginWidgetClass);
    }
    
    if (LoginWidget && !LoginWidget->IsInViewport())
    {
        LoginWidget->AddToViewport();
        
        // Simplified input mode for iOS
        FInputModeUIOnly InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        // Don't set widget to focus here - let the widget handle its own focus
        PC->SetInputMode(InputMode);
        
        #if PLATFORM_IOS
        PC->bShowMouseCursor = false;
        #endif
        
        // Let the widget set its own focus after a frame
        GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
        {
            if (LoginWidget && LoginWidget->PhoneBox)
            {
                LoginWidget->PhoneBox->SetKeyboardFocus();
            }
        });
    }
}

void ALoginActor::HideLoginUI()
{
    if (!IsInGameThread())
    {
        AsyncTask(ENamedThreads::GameThread, [this]()
        {
            HideLoginUI();  // Recursive call on game thread
        });
        return;
    }
    
    if (LoginWidget && LoginWidget->IsInViewport())  // Check if valid and in viewport
    {
        LoginWidget->RemoveFromParent();
        LoginWidget = nullptr;
    }
    
    // Return to game input
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)  // Good that you check this
    {
        FInputModeGameOnly GameMode;
        PC->SetInputMode(GameMode);
        PC->bShowMouseCursor = false;  // Good for iOS
        
        // Additional iOS-specific settings
        #if PLATFORM_IOS
        PC->bEnableClickEvents = false;
        PC->bEnableTouchEvents = true;
        #endif
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
        
        // Hide and destroy login UI before transitioning
        HideLoginUI();
        
        // Give a moment for UI to clean up, then spawn FurniLife
        FTimerHandle TransitionTimer;
        GetWorld()->GetTimerManager().SetTimer(TransitionTimer, [this]()
        {
            SpawnFurniLife();
        }, 0.5f, false);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Login] Invalid OTP"));
        // Stay on login screen for retry
    }
}
