#include "AuthSubsystem.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "ISLog.h"

namespace AuthService {
    static const FString DevMagicOtp(TEXT("123456"));
    static FString StartOtp(const FString& Phone, const FString& Name, FString& OutRequestId) {
        OutRequestId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
        UE_LOG(LogIntelliSpace, Log, TEXT("[Auth] DEV OTP %s -> %s (%s)"), *DevMagicOtp, *Phone, *Name);
        return OutRequestId;
    }
    static bool Verify(const FString& RequestId, const FString& Code, FISUserProfile& OutP) {
        if (Code != DevMagicOtp) return false;
        OutP.UserId = RequestId.Left(12);
        OutP.Token  = FGuid::NewGuid().ToString(EGuidFormats::Digits);
        return true;
    }
}

void UAuthSubsystem::Initialize(FSubsystemCollectionBase&){ JsonPath=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Auth/auth.json")); Load(); UE_LOG(LogIntelliSpace, Log, TEXT("[Auth] Initialize. LoggedIn=%d"), IsLoggedIn()); }
void UAuthSubsystem::Deinitialize(){ Save(); UE_LOG(LogIntelliSpace, Log, TEXT("[Auth] Deinitialize.")); }

void UAuthSubsystem::StartOtp(const FString& Phone, const FString& Name){
    FString ReqId; FString ok=AuthService::StartOtp(Phone,Name,ReqId);
    PendingRequestId = ok.IsEmpty() ? TEXT("") : ReqId; OnOtpStarted.Broadcast(!PendingRequestId.IsEmpty());
    UE_LOG(LogIntelliSpace, Log, TEXT("[Auth] StartOtp ok=%d"), !PendingRequestId.IsEmpty());
}
void UAuthSubsystem::VerifyOtp(const FString& Code){
    FISUserProfile NewP=Profile; const bool ok=AuthService::Verify(PendingRequestId,Code,NewP);
    if(ok){ Profile=NewP; Save(); } OnOtpVerified.Broadcast(ok);
    UE_LOG(LogIntelliSpace, Log, TEXT("[Auth] VerifyOtp %s"), ok?TEXT("OK"):TEXT("FAIL"));
}
void UAuthSubsystem::Logout(){
    Profile = {}; Save(); UE_LOG(LogIntelliSpace, Log, TEXT("[Auth] Logout -> cleared profile."));
}
void UAuthSubsystem::Save(){
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(JsonPath), true);
    auto O=MakeShared<FJsonObject>(); O->SetStringField("uid",Profile.UserId);
    O->SetStringField("name",Profile.Name); O->SetStringField("phone",Profile.Phone); O->SetStringField("token",Profile.Token);
    FString Out; auto W=TJsonWriterFactory<>::Create(&Out); FJsonSerializer::Serialize(O.ToSharedRef(),W); FFileHelper::SaveStringToFile(Out,*JsonPath);
    UE_LOG(LogIntelliSpace, Verbose, TEXT("[Auth] Saved %s"), *JsonPath);
}
void UAuthSubsystem::Load(){
    Profile={}; FString In; if(!FFileHelper::LoadFileToString(In,*JsonPath)) return;
    TSharedPtr<FJsonObject> O; auto R=TJsonReaderFactory<>::Create(In); if(!FJsonSerializer::Deserialize(R,O)||!O.IsValid()) return;
    Profile.UserId=O->GetStringField("uid"); Profile.Name=O->GetStringField("name"); Profile.Phone=O->GetStringField("phone"); Profile.Token=O->GetStringField("token");
}
