#include "LoginUIBoot.h"
#include "LoginWidget.h"
#include "AuthSaveGame.h"
#include "AuthSessionUtil.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

static FString MakeFakeToken(const FString& Phone)
{
    return FString::Printf(TEXT("tok_%s_%lld"), *Phone, FDateTime::UtcNow().GetTicks());
}

void ALoginUIBoot::BeginPlay()
{
    Super::BeginPlay();

    // 1) Auto-skip login if already authenticated
    if (UAuthSaveGame* Saved = AuthSession::Load())
    {
        if (Saved->IsValidSession())
        {
            GoToMain();
            return;
        }
    }

    // 2) Show login UI (first time or expired)
    if (!LoginWidgetClass) { UE_LOG(LogTemp, Error, TEXT("LoginWidgetClass not set")); return; }

    LoginWidget = CreateWidget<ULoginWidget>(GetWorld(), LoginWidgetClass);
    if (!LoginWidget) { UE_LOG(LogTemp, Error, TEXT("CreateWidget failed")); return; }

    LoginWidget->AddToViewport(1000);

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        FInputModeUIOnly Mode;
        Mode.SetWidgetToFocus(LoginWidget->TakeWidget());
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(Mode);
        PC->bShowMouseCursor = true;
    }

    LoginWidget->OnSendOtpRequested.AddDynamic(this, &ALoginUIBoot::HandleSendOtp);
    LoginWidget->OnVerifyOtpRequested.AddDynamic(this, &ALoginUIBoot::HandleVerifyOtp);
}

void ALoginUIBoot::HandleSendOtp(const FString& Phone, int32 OtpDigits)
{
    PendingPhone = Phone;

    // TODO (real app): call your backend to send OTP via SMS.
    // For now, generate a demo OTP and log it so you can test.
    ServerGeneratedOtp = TEXT("123456"); // Or generate randomly for local testing
    UE_LOG(LogTemp, Log, TEXT("📲 Demo OTP for %s is %s"), *Phone, *ServerGeneratedOtp);
}

void ALoginUIBoot::HandleVerifyOtp(const FString& Phone, const FString& OtpInput)
{
    // TODO (real app): call backend API to verify OTPInput.
    const bool bOk = (Phone == PendingPhone) && (OtpInput == ServerGeneratedOtp);

    if (!bOk)
    {
        UE_LOG(LogTemp, Warning, TEXT("OTP verify failed for %s"), *Phone);
        return;
    }

    // Create/save session
    UAuthSaveGame* SaveObj = Cast<UAuthSaveGame>(UGameplayStatics::CreateSaveGameObject(UAuthSaveGame::StaticClass()));
    SaveObj->UserId = Phone;
    SaveObj->AuthToken = MakeFakeToken(Phone); // replace with real token from server
    SaveObj->bLoggedIn = true;

    // Optional expiry: e.g., 365 days
    SaveObj->TokenExpiryUtc = FDateTime::UtcNow() + FTimespan(365,0,0,0);

    if (AuthSession::Save(SaveObj))
    {
        UE_LOG(LogTemp, Log, TEXT("✅ Logged in as %s; token saved"), *Phone);
        GoToMain();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save session"));
    }
}

void ALoginUIBoot::GoToMain()
{
    if (MainMapName.IsNone()) { UE_LOG(LogTemp, Error, TEXT("MainMapName not set")); return; }
    UGameplayStatics::OpenLevel(this, MainMapName);
}
