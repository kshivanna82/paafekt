// InteriorDesignActor.cpp (fixed)
#include "InteriorDesignActor.h"
#include "CoreMinimal.h"

AInteriorDesignActor::AInteriorDesignActor()
{
    PrimaryActorTick.bCanEverTick = true;
    UE_LOG(LogTemp, Log, TEXT("[InteriorDesign] Spawned"));
}

void AInteriorDesignActor::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Log, TEXT("[InteriorDesign] BeginPlay"));
}

void AInteriorDesignActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}
