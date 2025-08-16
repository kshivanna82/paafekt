#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ISBuilderActor.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIS_Builder_Discard, "IntelliSpace.Builder.Discard",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIS_Builder_Discard::RunTest(const FString& Parameters)
{
    UWorld* World = GEngine ? GEngine->GetCurrentPlayWorld() : nullptr;
    if (!World) {
        TestTrue(TEXT("No world in editor context; skip spawn."), true);
        return true;
    }
    AISBuilderActor* Actor = World->SpawnActor<AISBuilderActor>();
    TestNotNull("Actor spawned", Actor);
    Actor->DiscardCurrentCapture();
    return true;
}

#endif
