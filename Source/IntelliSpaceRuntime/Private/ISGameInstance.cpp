#include "ISGameInstance.h"

#include "IntelliSpaceRuntime.h"
#include "ISUserDataSaveGame.h"
#include "UI/FirstRunSlate.h"

#include "UObject/CoreUObjectDelegates.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraActor.h"

#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"

void UISGameInstance::Init()
{
    Super::Init();

    // Fire after any map loads (editor PIE & packaged)
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UISGameInstance::HandlePostLoadMap);
    UE_LOG(LogIntelliSpaceRuntime, Log, TEXT("GameInstance Init"));
}

void UISGameInstance::HandlePostLoadMap(UWorld* LoadedWorld)
{
    UE_LOG(LogIntelliSpaceRuntime, Log, TEXT("Map loaded: %s"), *GetNameSafe(LoadedWorld));

    EnsureCameraView(LoadedWorld);

    // Show first-run UI only once (if there is no save)
    if (!UGameplayStatics::DoesSaveGameExist(UISUserDataSaveGame::SlotName, UISUserDataSaveGame::UserIndex))
    {
        AddFirstRunUI();
    }
}

void UISGameInstance::EnsureCameraView(UWorld* World)
{
    if (!World) return;

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (!PC) return;

    // If the PC already has a view target (e.g., a spawned pawn), do nothing
    if (PC->GetViewTarget()) return;

    // Otherwise, pick the first CameraActor in the level
    TArray<AActor*> Cameras;
    UGameplayStatics::GetAllActorsOfClass(World, ACameraActor::StaticClass(), Cameras);
    if (Cameras.Num() > 0)
    {
        PC->SetViewTarget(Cameras[0]);
        UE_LOG(LogIntelliSpaceRuntime, Log, TEXT("Set view to first CameraActor"));
    }
    else
    {
        UE_LOG(LogIntelliSpaceRuntime, Warning, TEXT("No CameraActor found; view target unchanged."));
    }
}

void UISGameInstance::AddFirstRunUI()
{
    if (!GEngine || !GEngine->GameViewport) return;

    // Build the Slate widget and add to viewport
    TSharedRef<SFirstRunSlate> Panel =
        SNew(SFirstRunSlate)
        .OnSubmitted(FOnFirstRunSubmitted::CreateUObject(this, &UISGameInstance::OnFirstRunSubmitted));

    FirstRunRoot = Panel;
    GEngine->GameViewport->AddViewportWidgetContent(Panel);

    FSlateApplication::Get().SetAllUserFocus(Panel, EFocusCause::SetDirectly);
    UE_LOG(LogIntelliSpaceRuntime, Log, TEXT("First-run UI shown"));
}

void UISGameInstance::RemoveFirstRunUI()
{
    if (FirstRunRoot.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(FirstRunRoot.ToSharedRef());
        FirstRunRoot.Reset();
        UE_LOG(LogIntelliSpaceRuntime, Log, TEXT("First-run UI removed"));
    }
}

void UISGameInstance::OnFirstRunSubmitted(const FString& Name, const FString& Phone)
{
    // Save to slot
    if (UISUserDataSaveGame* Save = Cast<UISUserDataSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UISUserDataSaveGame::StaticClass())))
    {
        Save->UserName    = Name;
        Save->PhoneNumber = Phone;

        const bool bOK = UGameplayStatics::SaveGameToSlot(
            Save, UISUserDataSaveGame::SlotName, UISUserDataSaveGame::UserIndex);

        UE_LOG(LogIntelliSpaceRuntime, Log, TEXT("Saved first-run data: %s (phone hidden)  OK=%d"),
            *Name, bOK ? 1 : 0);
    }

    RemoveFirstRunUI();
}
