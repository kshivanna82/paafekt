// LoginActor.cpp (fixed)
#include "LoginActor.h"
#include "AuthSubsystem.h"

ALoginActor::ALoginActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ALoginActor::BeginPlay()
{
    Super::BeginPlay();
    UGameInstance* GI = GetGameInstance();
    Auth = GI ? GI->GetSubsystem<UAuthSubsystem>() : nullptr;
    if (!Auth)
    {
        UE_LOG(LogTemp, Error, TEXT("[Login] AuthSubsystem missing"));
        return;
    }

    Auth->OnOtpStarted.AddDynamic(this, &ALoginActor::OnOtpStarted);
    Auth->OnOtpVerified.AddDynamic(this, &ALoginActor::OnOtpVerified);
}

void ALoginActor::OnOtpStarted(bool bOk)
{
    UE_LOG(LogTemp, Log, TEXT("[Login] OTP start %s"), bOk ? TEXT("OK") : TEXT("FAIL"));
}

void ALoginActor::OnOtpVerified(bool bOk)
{
    UE_LOG(LogTemp, Log, TEXT("[Login] Verify %s"), bOk ? TEXT("OK") : TEXT("FAIL"));
    if (bOk) ProceedToMain();
}

void ALoginActor::ProceedToMain()
{
    UE_LOG(LogTemp, Log, TEXT("[Login] Proceeding to main mode"));
}
