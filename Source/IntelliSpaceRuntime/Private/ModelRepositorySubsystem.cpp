// ModelRepositorySubsystem.cpp (fixed full file)
#include "ModelRepositorySubsystem.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

void SerializeRepoRoot(const TSharedRef<FJsonObject>& Root, FString& OutJson)
{
    TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&OutJson);
    FJsonSerializer::Serialize(Root, W);
}

bool TryGetModelsArray(const TSharedRef<FJsonObject>& Root, const TArray<TSharedPtr<FJsonValue>>*& OutArray)
{
    return Root->TryGetArrayField(TEXT("models"), OutArray);
}

void ReadModelEntry(const TSharedRef<FJsonObject>& O, FString& OutName, FString& OutObj, FString& OutThumb)
{
    OutName  = O->GetStringField(TEXT("name"));
    OutObj   = O->GetStringField(TEXT("obj"));
    OutThumb = O->GetStringField(TEXT("thumb"));
}
