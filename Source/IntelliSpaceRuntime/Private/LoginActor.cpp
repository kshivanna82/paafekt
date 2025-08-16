#include "LoginActor.h"
#include "AuthSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Kismet/GameplayStatics.h"
#include "ISLog.h"

void ALoginActor::BeginPlay(){
    Super::BeginPlay();
    if(UAuthSubsystem* Auth=GetGameInstance()->GetSubsystem<UAuthSubsystem>()){
        if(Auth->IsLoggedIn()){ UE_LOG(LogIntelliSpace, Log, TEXT("[Login] Already logged in.")); ProceedToMain(); return; }
        Auth->OnOtpStarted.AddDynamic(this,[](bool bOk){ UE_LOG(LogIntelliSpace, Log, TEXT("[Login] OTP start %s"), bOk?TEXT("OK"):TEXT("FAIL")); });
        Auth->OnOtpVerified.AddDynamic(this,[this](bool bOk){ UE_LOG(LogIntelliSpace, Log, TEXT("[Login] Verify %s"), bOk?TEXT("OK"):TEXT("FAIL")); if(bOk) ProceedToMain(); });
    }
    if(LoginWidgetClass){
        LoginWidget=CreateWidget<UUserWidget>(GetWorld(),LoginWidgetClass); LoginWidget->AddToViewport(100);
        if (UButton* B=Cast<UButton>(LoginWidget->GetWidgetFromName(TEXT("BtnSend"))))   B->OnClicked.AddDynamic(this,&ALoginActor::OnSendClicked);
        if (UButton* B=Cast<UButton>(LoginWidget->GetWidgetFromName(TEXT("BtnVerify")))) B->OnClicked.AddDynamic(this,&ALoginActor::OnVerifyClicked);
        if (UButton* B=Cast<UButton>(LoginWidget->GetWidgetFromName(TEXT("BtnLogout")))) B->OnClicked.AddDynamic(this,&ALoginActor::OnLogoutClicked);
        UE_LOG(LogIntelliSpace, Log, TEXT("[Login] Widget created."));
    }
}

void ALoginActor::OnSendClicked(){
    if(!LoginWidget) return;
    auto P=Cast<UEditableText>(LoginWidget->GetWidgetFromName(TEXT("EdtPhone")));
    auto N=Cast<UEditableText>(LoginWidget->GetWidgetFromName(TEXT("EdtName")));
    const FString Phone = P?P->GetText().ToString():TEXT("");
    const FString Name  = N?N->GetText().ToString():TEXT("");
    if(UAuthSubsystem* Auth=GetGameInstance()->GetSubsystem<UAuthSubsystem>()){ Auth->StartOtp(Phone,Name); }
    UE_LOG(LogIntelliSpace, Log, TEXT("[Login] Send OTP: %s (%s)"), *Phone, *Name);
}

void ALoginActor::OnVerifyClicked(){
    if(!LoginWidget) return;
    auto O=Cast<UEditableText>(LoginWidget->GetWidgetFromName(TEXT("EdtOtp")));
    const FString Code = O?O->GetText().ToString():TEXT("");
    if(UAuthSubsystem* Auth=GetGameInstance()->GetSubsystem<UAuthSubsystem>()){ Auth->VerifyOtp(Code); }
    UE_LOG(LogIntelliSpace, Log, TEXT("[Login] Verify OTP: %s"), *Code);
}

void ALoginActor::OnLogoutClicked(){
    if(UAuthSubsystem* Auth=GetGameInstance()->GetSubsystem<UAuthSubsystem>()){ Auth->Logout(); }
    UE_LOG(LogIntelliSpace, Log, TEXT("[Login] Logout clicked."));
}

void ALoginActor::ProceedToMain(){ if(!MainLevelName.IsNone()) UGameplayStatics::OpenLevel(this, MainLevelName); }
