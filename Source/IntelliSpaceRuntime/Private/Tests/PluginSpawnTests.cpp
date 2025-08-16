
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "ISLog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIS_Plugin_Spawn, "IntelliSpace.Plugin.InteriorDesign.SpawnClass",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIS_Plugin_Spawn::RunTest(const FString& Parameters)
{
    UClass* InteriorClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Script/InteriorDesignPlugin.InteriorDesignActor"));
    TestNotNull("InteriorDesignActor class loads", InteriorClass);
    return true;
}
#endif
