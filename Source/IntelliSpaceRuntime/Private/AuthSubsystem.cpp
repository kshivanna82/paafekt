#include "AuthSubsystem.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

static FString EnsureDirAndReturnFile(const FString& FilePath)
{
    const FString Dir = FPaths::GetPath(FilePath);
    IFileManager::Get().MakeDirectory(*Dir, /*Tree*/true);
    return FilePath;
}

FString UAuthSubsystem::DefaultJsonPath()
{
    return FPaths::ProjectSavedDir() / TEXT("Auth/Profile.json");
}

void UAuthSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    JsonPath = DefaultJsonPath();
    LoadFromDisk();
    UE_LOG(LogTemp, Log, TEXT("[Auth] Initialize (LoggedIn=%s, Path=%s)"),
        IsLoggedIn() ? TEXT("true") : TEXT("false"), *JsonPath);
}

void UAuthSubsystem::Deinitialize()
{
    SaveToDisk();
    UE_LOG(LogTemp, Log, TEXT("[Auth] Deinitialize"));
    Super::Deinitialize();
}

void UAuthSubsystem::__Test_SetJsonPath(const FString& NewPath)
{
    JsonPath = EnsureDirAndReturnFile(NewPath);
    // After changing path, reload if a file exists; otherwise keep current state
    FString Dummy;
    if (FPaths::FileExists(JsonPath) && FFileHelper::LoadFileToString(Dummy, *JsonPath))
    {
        LoadFromDisk();
    }
}

void UAuthSubsystem::__Test_ClearProfile()
{
    Profile = FAuthProfile();
    SaveToDisk();
}

void UAuthSubsystem::StartOtp(const FString& PhoneNumber, const FString& CountryCode)
{
    // Minimal behavior for tests: record phone, simulate request success.
    Profile.Phone = PhoneNumber;
    // Keep any existing uid/name/token until VerifyOtp.
    SaveToDisk();

    UE_LOG(LogTemp, Log, TEXT("[Auth] StartOtp phone=%s cc=%s"), *PhoneNumber, *CountryCode);
    OnOtpStarted.Broadcast(true);
}

void UAuthSubsystem::VerifyOtp(const FString& OtpCode)
{
    // Minimal behavior for tests: set a token and dummy identity.
    if (Profile.UserId.IsEmpty())  Profile.UserId = TEXT("uid-test");
    if (Profile.Name.IsEmpty())    Profile.Name   = TEXT("Test User");

    // Use OTP code as token stub so tests can assert non-empty.
    Profile.Token = OtpCode.IsEmpty() ? TEXT("TEST_TOKEN") : OtpCode;

    SaveToDisk();

    UE_LOG(LogTemp, Log, TEXT("[Auth] VerifyOtp -> LoggedIn=true (token len %d)"), Profile.Token.Len());
    OnOtpVerified.Broadcast(true);
}

void UAuthSubsystem::Logout()
{
    UE_LOG(LogTemp, Log, TEXT("[Auth] Logout"));
    Profile.Token.Reset();
    SaveToDisk();
}

void UAuthSubsystem::LoadFromDisk()
{
    const FString Path = EnsureDirAndReturnFile(JsonPath.IsEmpty() ? DefaultJsonPath() : JsonPath);

    FString JsonStr;
    if (!FFileHelper::LoadFileToString(JsonStr, *Path))
    {
        // Nothing to load yet; keep defaults.
        return;
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Auth] Failed to parse %s"), *Path);
        return;
    }

    Root->TryGetStringField(TEXT("uid"),   Profile.UserId);
    Root->TryGetStringField(TEXT("name"),  Profile.Name);
    Root->TryGetStringField(TEXT("phone"), Profile.Phone);
    Root->TryGetStringField(TEXT("token"), Profile.Token);
}

void UAuthSubsystem::SaveToDisk() const
{
    const FString Path = EnsureDirAndReturnFile(JsonPath.IsEmpty() ? DefaultJsonPath() : JsonPath);

    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("uid"),   Profile.UserId);
    Root->SetStringField(TEXT("name"),  Profile.Name);
    Root->SetStringField(TEXT("phone"), Profile.Phone);
    Root->SetStringField(TEXT("token"), Profile.Token);

    FString Out;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
    FJsonSerializer::Serialize(Root, Writer);

    if (!FFileHelper::SaveStringToFile(Out, *Path))
    {
        UE_LOG(LogTemp, Warning, TEXT("[Auth] Failed to save %s"), *Path);
    }
}
