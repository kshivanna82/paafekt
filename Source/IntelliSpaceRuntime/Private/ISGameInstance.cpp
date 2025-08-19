#include "ISGameInstance.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
//#include "UObject/CoreUObjectDelegates.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"

#include "Kismet/GameplayStatics.h"
#include "UI/FirstRunSlate.h"
#include "ISUserDataSaveGame.h"

void UISGameInstance::Init()
{
    Super::Init();

    // Fire every time a new world/map is loaded
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
        this, &UISGameInstance::OnPostLoadMapWithWorld);
}

void UISGameInstance::OnStart()
{
    Super::OnStart();
    // Optional: you could open a specific map here if desired.
    // UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Maps/YourStartupMap")));
}

void UISGameInstance::OnPostLoadMapWithWorld(UWorld* /*LoadedWorld*/)
{
    // Only show the first-run UI if we don't already have data
    FString Name, Phone;
    if (!LoadUserData(Name, Phone))
    {
        AddFirstRunUI();
    }
}

void UISGameInstance::AddFirstRunUI()
{
    if (!GEngine || !GEngine->GameViewport || FirstRunUI.IsValid())
    {
        return;
    }

    // Build the Slate widget
    TSharedRef<SFirstRunSlate> WidgetRef =
        SNew(SFirstRunSlate)
        .OnSubmitted(FOnFirstRunSubmitted::CreateUObject(
            this, &UISGameInstance::OnFirstRunSubmitted));

    FirstRunUI = WidgetRef;

    // Add to the viewport
    GEngine->GameViewport->AddViewportWidgetContent(FirstRunUI.ToSharedRef(), /*ZOrder*/ 100);

    // Give it keyboard focus
    FSlateApplication::Get().SetKeyboardFocus(WidgetRef);
}

void UISGameInstance::RemoveFirstRunUI()
{
    if (GEngine && GEngine->GameViewport && FirstRunUI.IsValid())
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(FirstRunUI.ToSharedRef());
        FirstRunUI.Reset();
    }
}

void UISGameInstance::OnFirstRunSubmitted(const FString& Name, const FString& Phone)
{
    SaveUserData(Name, Phone);
    RemoveFirstRunUI();

    // Continue your normal startup flow here if needed (load map, show main UI, etc.)
}

bool UISGameInstance::LoadUserData(FString& OutName, FString& OutPhone) const
{
    if (USaveGame* Base = UGameplayStatics::LoadGameFromSlot(FirstRunSlot, UserIndex))
    {
        if (const UISUserDataSaveGame* Typed = Cast<UISUserDataSaveGame>(Base))
        {
            OutName  = Typed->UserName;
            OutPhone = Typed->Phone;
            return !OutName.IsEmpty() && !OutPhone.IsEmpty();
        }
    }
    return false;
}

bool UISGameInstance::SaveUserData(const FString& Name, const FString& Phone) const
{
    UISUserDataSaveGame* SaveObj = nullptr;

    if (USaveGame* Base = UGameplayStatics::LoadGameFromSlot(FirstRunSlot, UserIndex))
    {
        SaveObj = Cast<UISUserDataSaveGame>(Base);
    }

    if (!SaveObj)
    {
        SaveObj = Cast<UISUserDataSaveGame>(UGameplayStatics::CreateSaveGameObject(UISUserDataSaveGame::StaticClass()));
    }
    if (!SaveObj) return false;

    SaveObj->UserName = Name;
    SaveObj->Phone    = Phone;

    return UGameplayStatics::SaveGameToSlot(SaveObj, FirstRunSlot, UserIndex);
}
