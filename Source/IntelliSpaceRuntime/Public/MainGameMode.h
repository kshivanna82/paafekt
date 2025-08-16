#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SegDecorActor.h"
#include "ISBuilderActor.h"
#include "ModeSubsystem.h"
#include "MainGameMode.generated.h"

UCLASS()
class AMainGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, Category="Startup") TSubclassOf<ASegDecorActor> SegDecorActorClass;
    UPROPERTY(EditDefaultsOnly, Category="Startup") TSubclassOf<AISBuilderActor> BuilderActorClass;

    virtual void BeginPlay() override;
};
