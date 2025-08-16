#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ModelRepositorySubsystem.generated.h"

USTRUCT(BlueprintType)
struct FISModelEntry {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString ObjPath;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString ThumbnailPath;
};

UCLASS()
class UModelRepositorySubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Coll) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable) const TArray<FISModelEntry>& GetModels() const { return Models; }
    UFUNCTION(BlueprintCallable) void AddModel(const FISModelEntry& Entry);
    UFUNCTION(BlueprintCallable) bool RemoveModelByName(const FString& Name, bool bDeleteFiles=true);
    UFUNCTION(BlueprintCallable) bool RemoveModelByPath(const FString& ObjPath, bool bDeleteFiles=true);
    UFUNCTION(BlueprintCallable) void Save();
    UFUNCTION(BlueprintCallable) void Load();

#if WITH_DEV_AUTOMATION_TESTS
    void __Test_SetJsonPath(const FString& InPath) { JsonPath = InPath; }
#endif

private:
    TArray<FISModelEntry> Models;
    FString JsonPath;
};
