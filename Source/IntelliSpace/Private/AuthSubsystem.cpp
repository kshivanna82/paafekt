// AuthSubsystem.cpp
#include "AuthSubsystem.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

void UAuthSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LoadFromDisk();
    UE_LOG(LogTemp, Log, TEXT("[Auth] Initialize (LoggedIn=%s)"), IsLoggedIn() ? TEXT("true") : TEXT("false"));
}

void UAuthSubsystem::Deinitialize()
{
    SaveToDisk();
    Super::Deinitialize();
}

void UAuthSubsystem::StartOtp(const FString& PhoneNumber, const FString& CountryCode)
{
    // For testing: Just save phone and trigger success
    Profile.Phone = PhoneNumber;
    SaveToDisk();
    
    UE_LOG(LogTemp, Log, TEXT("[Auth] StartOtp phone=%s"), *PhoneNumber);
    OnOtpStarted.Broadcast(true);
}

void UAuthSubsystem::VerifyOtp(const FString& OtpCode)
{
    // For testing: Accept any 6-digit code
    if (OtpCode.Len() == 6)
    {
        Profile.UserId = TEXT("user_123");
        Profile.Name = TEXT("Test User");
        Profile.Token = TEXT("test_token_") + OtpCode;
        SaveToDisk();
        
        UE_LOG(LogTemp, Log, TEXT("[Auth] VerifyOtp success"));
        OnOtpVerified.Broadcast(true);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Auth] Invalid OTP"));
        OnOtpVerified.Broadcast(false);
    }
}

void UAuthSubsystem::Logout()
{
    Profile.Token.Empty();
    SaveToDisk();
    UE_LOG(LogTemp, Log, TEXT("[Auth] Logged out"));
}

FString UAuthSubsystem::GetSavePath()
{
    return FPaths::ProjectSavedDir() / TEXT("Auth/Profile.json");
}

void UAuthSubsystem::LoadFromDisk()
{
    FString JsonStr;
    if (!FFileHelper::LoadFileToString(JsonStr, *GetSavePath()))
    {
        return;
    }
    
    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
    {
        Root->TryGetStringField(TEXT("uid"), Profile.UserId);
        Root->TryGetStringField(TEXT("name"), Profile.Name);
        Root->TryGetStringField(TEXT("phone"), Profile.Phone);
        Root->TryGetStringField(TEXT("token"), Profile.Token);
    }
}

void UAuthSubsystem::SaveToDisk() const
{
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("uid"), Profile.UserId);
    Root->SetStringField(TEXT("name"), Profile.Name);
    Root->SetStringField(TEXT("phone"), Profile.Phone);
    Root->SetStringField(TEXT("token"), Profile.Token);
    
    FString Out;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
    FJsonSerializer::Serialize(Root, Writer);
    
    FString Path = GetSavePath();
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
    FFileHelper::SaveStringToFile(Out, *Path);
}
