// AuthSubsystem.cpp (fixed full file)
#include "AuthSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

void SaveAuthJson(const TSharedRef<FJsonObject>& O, const FString& JsonPath)
{
    FString Out;
    TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
    FJsonSerializer::Serialize(O, W);
    FFileHelper::SaveStringToFile(Out, *JsonPath);
}

void ReadProfileFields(const TSharedRef<FJsonObject>& O, FString& OutUid, FString& OutName, FString& OutPhone, FString& OutToken)
{
    OutUid   = O->GetStringField(TEXT("uid"));
    OutName  = O->GetStringField(TEXT("name"));
    OutPhone = O->GetStringField(TEXT("phone"));
    OutToken = O->GetStringField(TEXT("token"));
}
