#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "PlatformCameraCaptureBridge.h"
#include "MiDaSRunnerBridge.h"
#include "U2NetRunnerBridge.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIS_Bridge_Smoke, "IntelliSpace.Bridges.Smoke",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIS_Bridge_Smoke::RunTest(const FString& Parameters)
{
    RegisterCameraHandler(nullptr, nullptr);
    (void)StartCameraCapture();
    StopCameraCapture();

    void* M = CreateMiDaSRunner(); DestroyMiDaSRunner(M);
    void* S = CreateU2NetRunner(); DestroyU2NetRunner(S);

    return true;
}

#endif
