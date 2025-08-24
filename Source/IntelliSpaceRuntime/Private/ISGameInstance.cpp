#include "ISGameInstance.h" // must be first

#include "UI/FirstRunSlate.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Camera/CameraActor.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"

void UISGameInstance::OnStart()
{
    Super::OnStart();

    // Ensure a valid view so we don't get a black screen
    EnsureCameraView();

    // Optional panel
    ShowFirstRunIfNeeded();
}

void UISGameInstance::ShowFirstRunIfNeeded()
{
    if (!IsFirstRun())
    {
        return;
    }

    TSharedRef<SFirstRunSlate> Panel = SNew(SFirstRunSlate)
        .OnSubmit(FOnSubmit::CreateUObject(this, &UISGameInstance::OnFirstRunSubmitted));


    FirstRunWindow = SNew(SWindow)
        .Title(FText::FromString(TEXT("Welcome")))
        .SizingRule(ESizingRule::Autosized)
        .AutoCenter(EAutoCenter::PreferredWorkArea)
        .SupportsMaximize(false)
        .SupportsMinimize(false)
    [
        Panel
    ];

    FSlateApplication::Get().AddWindow(FirstRunWindow.ToSharedRef());
}

void UISGameInstance::OnFirstRunSubmitted()
{
    if (FirstRunWindow.IsValid())
    {
        FirstRunWindow->RequestDestroyWindow();
        FirstRunWindow.Reset();
    }
}

void UISGameInstance::EnsureCameraView()
{
    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return;

    // If no view target yet, try to use any CameraActor placed in the level
    if (!PC->GetViewTarget())
    {
        for (TActorIterator<ACameraActor> It(World); It; ++It)
        {
            PC->SetViewTarget(*It);
            break;
        }
    }
}

bool UISGameInstance::IsFirstRun() const
{
    // Replace with real check if needed
    return false;
}
