
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "InteriorDesignActor.generated.h"

USTRUCT(BlueprintType)
struct FDesignItem
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString MeshPath;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FTransform Transform;
};

UCLASS(BlueprintType, Blueprintable)
class AInteriorDesignActor : public AActor
{
    GENERATED_BODY()
public:
    AInteriorDesignActor();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    UFUNCTION(BlueprintCallable, Category="Design") void CreateGridFloor(int32 HalfSize=500, int32 Cells=10, float Z=0.f);
    UFUNCTION(BlueprintCallable, Category="Design") AActor* AddFurniture(class UStaticMesh* Mesh, const FTransform& Xform);
    UFUNCTION(BlueprintCallable, Category="Design") void RemoveAllFurniture();
    UFUNCTION(BlueprintCallable, Category="Design") bool SaveLayout(const FString& Name = TEXT("layout.json"));
    UFUNCTION(BlueprintCallable, Category="Design") bool LoadLayout(const FString& Name = TEXT("layout.json"));

    UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* FloorMesh;

private:
    TArray<AActor*> Placed;
    FString LayoutDir;
};
