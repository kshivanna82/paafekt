#include "ISGameInstance.h"
#include "ISUserDataSaveGame.h"
#include "UI/FirstRunSlate.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

#include "Widgets/SWeakWidget.h"        // <-- required
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SOverlay.h"

static const TCHAR* FirstRunSlot = TEXT("UserData");
static const int32 UserIndex = 0;

//void UISGameInstance::Init()
//{
//    Super::Init();
//    UE_LOG(LogTemp, Log, TEXT("UISGameInstance::Init"));
//    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
//        this, &UISGameInstance::HandlePostLoadMap);
//}

//void UISGameInstance::OnStart()
//{
//    Super::OnStart();
//    UE_LOG(LogTemp, Log, TEXT("UISGameInstance::OnStart"));
//    if (GEngine && GEngine->GameViewport)
//    {
//        AddFirstRunUI(); // your function that adds the Slate widget
//    }
//}

//void UISGameInstance::HandlePostLoadMap(UWorld*)
//{
//    UE_LOG(LogTemp, Log, TEXT("UISGameInstance::HandlePostLoadMap"));
//    AddFirstRunUI();
//}


void UISGameInstance::OnStart()
{
	Super::OnStart();
    
    if (GEngine && GEngine->GameViewport)
    {
        TSharedRef<STextBlock> DebugText =
            SNew(STextBlock).Text(FText::FromString(TEXT("Rendering OK")))
                            .Justification(ETextJustify::Center)
                            .Font(FSlateFontInfo("Verdana", 28));
        GEngine->GameViewport->AddViewportWidgetContent(
            SNew(SWeakWidget).PossiblyNullContent(DebugText), 1000);
    }


	if (!HasCompletedFirstRun())
	{
		ShowFirstRun();
	}
}

bool UISGameInstance::HasCompletedFirstRun() const
{
	if (UGameplayStatics::DoesSaveGameExist(FirstRunSlot, UserIndex))
	{
		if (auto* Save = Cast<UISUserDataSaveGame>(UGameplayStatics::LoadGameFromSlot(FirstRunSlot, UserIndex)))
		{
			return Save->bCompleted;
		}
	}
	return false;
}

void UISGameInstance::ShowFirstRun()
{
	if (!GEngine || !GEngine->GameViewport) { return; }

	TSharedRef<SFirstRunSlate> Widget =
		SNew(SFirstRunSlate)
        .OnSubmitted(FOnFirstRunSubmitted::CreateUObject(this, &UISGameInstance::OnFirstRunSubmitted));

	FirstRunWidget = Widget;
	GEngine->GameViewport->AddViewportWidgetContent(Widget, 10000 /*z-order*/);
}

void UISGameInstance::OnFirstRunSubmitted(const FString& Name, const FString& Phone)
{
	UISUserDataSaveGame* Save = nullptr;

	if (UGameplayStatics::DoesSaveGameExist(FirstRunSlot, UserIndex))
	{
		Save = Cast<UISUserDataSaveGame>(UGameplayStatics::LoadGameFromSlot(FirstRunSlot, UserIndex));
	}

	if (!Save)
	{
		Save = Cast<UISUserDataSaveGame>(UGameplayStatics::CreateSaveGameObject(UISUserDataSaveGame::StaticClass()));
	}

	if (Save)
	{
		Save->Name = Name;
		Save->Phone = Phone;
		Save->bCompleted = true;
		UGameplayStatics::SaveGameToSlot(Save, FirstRunSlot, UserIndex);
	}

	// Remove the widget
	if (GEngine && GEngine->GameViewport && FirstRunWidget.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(FirstRunWidget.ToSharedRef());
		FirstRunWidget.Reset();
	}
}
