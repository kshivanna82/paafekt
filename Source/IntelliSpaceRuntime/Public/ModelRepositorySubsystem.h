#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ModelRepositorySubsystem.generated.h"

USTRUCT(BlueprintType)
struct INTELLISPACERUNTIME_API FISModelEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString ObjPath;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString ThumbnailPath;
};

UCLASS(BlueprintType)
class INTELLISPACERUNTIME_API UModelRepositorySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="Models")
    void AddModel(const FISModelEntry& Entry);

    UFUNCTION(BlueprintCallable, Category="Models")
    void RemoveModelByName(const FString& Name, bool bFirstOnly);

    UFUNCTION(BlueprintCallable, Category="Models")
    void RemoveModelByPath(const FString& ObjPath, bool bFirstOnly);

    UFUNCTION(BlueprintCallable, Category="Models|IO")
    void Save();

    UFUNCTION(BlueprintCallable, Category="Models|IO")
    void Load();

    const TArray<FISModelEntry>& GetModels() const { return Models; }

private:
    TArray<FISModelEntry> Models;
    FString RepoPath; // Saved directory JSON
};
