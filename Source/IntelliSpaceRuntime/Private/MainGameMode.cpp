#include "MainGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "ModeSubsystem.h"
#include "ISLog.h"

void AMainGameMode::BeginPlay(){
    Super::BeginPlay();
    UModeSubsystem* Mode = GetGameInstance()->GetSubsystem<UModeSubsystem>();
    const EISAppMode M = Mode ? Mode->GetMode() : EISAppMode::SegAndDecor;

    if (M == EISAppMode::SegAndDecor) {
        TArray<AActor*> Found; UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASegDecorActor::StaticClass(), Found);
        if(Found.Num()==0 && SegDecorActorClass){
            GetWorld()->SpawnActor<ASegDecorActor>(SegDecorActorClass, FVector::ZeroVector, FRotator::ZeroRotator);
            UE_LOG(LogIntelliSpace, Log, TEXT("[MainGameMode] Spawned SegDecorActor."));
        }
    } else if (M == EISAppMode::Builder3D) {
        if (BuilderActorClass){
            GetWorld()->SpawnActor<AISBuilderActor>(BuilderActorClass, FVector::ZeroVector, FRotator::ZeroRotator);
            UE_LOG(LogIntelliSpace, Log, TEXT("[MainGameMode] Spawned ISBuilderActor."));
        }
    } else if (M == EISAppMode::FutureInteriorDesign) {
        UClass* InteriorClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Script/InteriorDesignPlugin.InteriorDesignActor")); 
        if (InteriorClass) { GetWorld()->SpawnActor<AActor>(InteriorClass, FVector::ZeroVector, FRotator::ZeroRotator); UE_LOG(LogIntelliSpace, Log, TEXT("[MainGameMode] Spawned InteriorDesignActor (plugin).")); }
        else { UE_LOG(LogIntelliSpace, Warning, TEXT("[MainGameMode] Could not load InteriorDesignActor class.")); }
    }
}
