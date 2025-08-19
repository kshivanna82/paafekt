#include "ModelRepositorySubsystem.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

static FString GetRepoFile()
{
    const FString Dir = FPaths::ProjectSavedDir() / TEXT("ModelRepository");
    IFileManager::Get().MakeDirectory(*Dir, /*Tree*/true);
    return Dir / TEXT("Models.json");
}

void UModelRepositorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    RepoPath = GetRepoFile();
    Load();
    UE_LOG(LogTemp, Log, TEXT("[Models] Initialize: %d entries"), Models.Num());
}

void UModelRepositorySubsystem::Deinitialize()
{
    Save();
    UE_LOG(LogTemp, Log, TEXT("[Models] Deinitialize"));
    Super::Deinitialize();
}

void UModelRepositorySubsystem::AddModel(const FISModelEntry& Entry)
{
    Models.Add(Entry);
}

void UModelRepositorySubsystem::RemoveModelByName(const FString& Name, bool bFirstOnly)
{
    for (int32 i = Models.Num()-1; i >= 0; --i)
    {
        if (Models[i].Name.Equals(Name, ESearchCase::IgnoreCase))
        {
            Models.RemoveAt(i);
            if (bFirstOnly) return;
        }
    }
}

void UModelRepositorySubsystem::RemoveModelByPath(const FString& ObjPath, bool bFirstOnly)
{
    for (int32 i = Models.Num()-1; i >= 0; --i)
    {
        if (Models[i].ObjPath.Equals(ObjPath, ESearchCase::IgnoreCase))
        {
            Models.RemoveAt(i);
            if (bFirstOnly) return;
        }
    }
}

void UModelRepositorySubsystem::Save()
{
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Arr;
    Arr.Reserve(Models.Num());

    for (const FISModelEntry& M : Models)
    {
        TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
        O->SetStringField(TEXT("name"), M.Name);
        O->SetStringField(TEXT("obj"), M.ObjPath);
        O->SetStringField(TEXT("thumb"), M.ThumbnailPath);
        Arr.Add(MakeShared<FJsonValueObject>(O));
    }

    Root->SetArrayField(TEXT("models"), Arr);

    FString Out;
    TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
    FJsonSerializer::Serialize(Root, W);

    if (FFileHelper::SaveStringToFile(Out, *RepoPath))
    {
        UE_LOG(LogTemp, Log, TEXT("[Models] Saved %d to %s"), Models.Num(), *RepoPath);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Models] Failed to save %s"), *RepoPath);
    }
}

void UModelRepositorySubsystem::Load()
{
    Models.Reset();

    FString In;
    if (!FFileHelper::LoadFileToString(In, *RepoPath))
    {
        UE_LOG(LogTemp, Log, TEXT("[Models] No repo to load at %s (this is fine)"), *RepoPath);
        return;
    }

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(In);
    if (!FJsonSerializer::Deserialize(R, Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Models] JSON parse failed: %s"), *RepoPath);
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
    if (!Root->TryGetArrayField(TEXT("models"), Arr) || !Arr)
    {
        return;
    }

    for (const TSharedPtr<FJsonValue>& V : *Arr)
    {
        const TSharedPtr<FJsonObject>* OPtr = nullptr;
        if (!V.IsValid() || !V->TryGetObject(OPtr) || !OPtr) continue;
        const TSharedPtr<FJsonObject>& O = *OPtr;

        FISModelEntry M;
        O->TryGetStringField(TEXT("name"), M.Name);
        O->TryGetStringField(TEXT("obj"), M.ObjPath);
        O->TryGetStringField(TEXT("thumb"), M.ThumbnailPath);
        Models.Add(M);
    }

    UE_LOG(LogTemp, Log, TEXT("[Models] Loaded %d from %s"), Models.Num(), *RepoPath);
}
