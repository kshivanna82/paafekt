#include "ModelRepositorySubsystem.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "ISLog.h"

void UModelRepositorySubsystem::Initialize(FSubsystemCollectionBase&){
    JsonPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Models/models.json"));
    Load();
    UE_LOG(LogIntelliSpace, Log, TEXT("[Repo] Initialize. Count=%d"), Models.Num());
}
void UModelRepositorySubsystem::Deinitialize(){ Save(); UE_LOG(LogIntelliSpace, Log, TEXT("[Repo] Deinitialize.")); }
void UModelRepositorySubsystem::AddModel(const FISModelEntry& E){ Models.Add(E); Save(); UE_LOG(LogIntelliSpace, Log, TEXT("[Repo] Add %s"), *E.Name); }

bool UModelRepositorySubsystem::RemoveModelByName(const FString& Name, bool bDeleteFiles){
    int32 idx = Models.IndexOfByPredicate([&](const FISModelEntry& M){ return M.Name == Name; });
    if (idx == INDEX_NONE) return false;
    if (bDeleteFiles){
        if (!Models[idx].ObjPath.IsEmpty()) IFileManager::Get().Delete(*Models[idx].ObjPath);
        if (!Models[idx].ThumbnailPath.IsEmpty()) IFileManager::Get().Delete(*Models[idx].ThumbnailPath);
    }
    UE_LOG(LogIntelliSpace, Log, TEXT("[Repo] Remove %s (delete=%d)"), *Name, bDeleteFiles);
    Models.RemoveAt(idx); Save(); return true;
}

bool UModelRepositorySubsystem::RemoveModelByPath(const FString& ObjPath, bool bDeleteFiles){
    int32 idx = Models.IndexOfByPredicate([&](const FISModelEntry& M){ return M.ObjPath == ObjPath; });
    if (idx == INDEX_NONE) return false;
    if (bDeleteFiles){
        if (!Models[idx].ObjPath.IsEmpty()) IFileManager::Get().Delete(*Models[idx].ObjPath);
        if (!Models[idx].ThumbnailPath.IsEmpty()) IFileManager::Get().Delete(*Models[idx].ThumbnailPath);
    }
    UE_LOG(LogIntelliSpace, Log, TEXT("[Repo] Remove by path %s (delete=%d)"), *ObjPath, bDeleteFiles);
    Models.RemoveAt(idx); Save(); return true;
}

void UModelRepositorySubsystem::Save(){
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(JsonPath), true);
    TArray<TSharedPtr<FJsonValue>> Arr;
    for(const auto& M:Models){ auto O=MakeShared<FJsonObject>();
        O->SetStringField("name",M.Name); O->SetStringField("obj",M.ObjPath); O->SetStringField("thumb",M.ThumbnailPath);
        Arr.Add(MakeShared<FJsonValueObject>(O)); }
    auto Root=MakeShared<FJsonObject>(); Root->SetArrayField("models",Arr);
    FString Out; auto W=TJsonWriterFactory<>::Create(&Out); FJsonSerializer::Serialize(Root.ToSharedRef(), W);
    FFileHelper::SaveStringToFile(Out,*JsonPath);
    UE_LOG(LogIntelliSpace, Log, TEXT("[Repo] Saved %s"), *JsonPath);
}
void UModelRepositorySubsystem::Load(){
    Models.Empty(); FString In; if(!FFileHelper::LoadFileToString(In,*JsonPath)) return;
    TSharedPtr<FJsonObject> Root; auto R=TJsonReaderFactory<>::Create(In);
    if(!FJsonSerializer::Deserialize(R,Root)||!Root.IsValid()) return;
    const TArray<TSharedPtr<FJsonValue>>* Arr=nullptr; if(!Root->TryGetArrayField("models",Arr)) return;
    for(auto& V:*Arr){ auto O=V->AsObject(); if(!O.IsValid()) continue;
        FISModelEntry M; M.Name=O->GetStringField("name"); M.ObjPath=O->GetStringField("obj"); M.ThumbnailPath=O->GetStringField("thumb");
        Models.Add(M); }
}
