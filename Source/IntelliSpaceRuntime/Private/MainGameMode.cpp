#include "MainGameMode.h"

#include "ModeSubsystem.h"          // UModeSubsystem
#include "Engine/GameInstance.h"    // UGameInstance + GetSubsystem<T>
#include "Engine/World.h"           // UWorld + SpawnActor on world
#include "Camera/CameraActor.h"        // <-- REQUIRED for ACameraActor
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h" // GetAllActorsOfClass

#include "SegDecorActor.h"          // ASegDecorActor (template & StaticClass use)
#include "ISBuilderActor.h"         // AISBuilderActor (if you spawn it)
#include "ISLog.h"

void AMainGameMode::BeginPlay(){
    Super::BeginPlay();
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC && !PC->GetViewTarget())
    {
        ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>();
        Cam->SetActorLocation(FVector(0, -300, 200));
        Cam->SetActorRotation(FRotator(-10, 0, 0));
        PC->SetViewTarget(Cam);
    }
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
