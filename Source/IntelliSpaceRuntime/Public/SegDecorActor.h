#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SegDecorActor.generated.h"

UCLASS()
class INTELLISPACERUNTIME_API ASegDecorActor : public AActor
{
    GENERATED_BODY()

public:
    // Match the .cpp signature exactly; default to FObjectInitializer::Get()
    ASegDecorActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
    virtual void BeginPlay() override;
};
