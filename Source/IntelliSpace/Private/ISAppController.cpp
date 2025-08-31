// ISAppController.cpp
#include "ISAppController.h"
#include "LoginActor.h"
#include "FurniLife.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"   
#include "Kismet/GameplayStatics.h"

AISAppController* AISAppController::Instance = nullptr;

AISAppController::AISAppController()
{
    PrimaryActorTick.bCanEverTick = false;
    CurrentState = EAppState::Login;
}

void AISAppController::BeginPlay()
{
    Super::BeginPlay();
    Instance = this;
    ChangeAppState(EAppState::Login);
}

AISAppController* AISAppController::GetInstance(UObject* WorldContextObject)
{
    if (!Instance && WorldContextObject)
    {
        UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
        if (World)
        {
            for (TActorIterator<AISAppController> It(World); It; ++It)
            {
                Instance = *It;
                break;
            }
        }
    }
    return Instance;
}

void AISAppController::ChangeAppState(EAppState NewState)
{
    if (CurrentState == NewState) return;
    
    UWorld* World = GetWorld();
    if (!World) return;
    
    // Cleanup current state
    switch (CurrentState)
    {
        case EAppState::Login:
            if (LoginActor)
            {
                LoginActor->Destroy();
                LoginActor = nullptr;
            }
            break;
            
        case EAppState::Segmentation:
            if (FurniLifeActor)
            {
                FurniLifeActor->Destroy();
                FurniLifeActor = nullptr;
            }
            break;
    }
    
    CurrentState = NewState;
    
    // Initialize new state
    switch (CurrentState)
    {
        case EAppState::Login:
            LoginActor = World->SpawnActor<ALoginActor>();
            break;
            
        case EAppState::Segmentation:
            FurniLifeActor = World->SpawnActor<AFurniLife>();
            break;
    }
}
