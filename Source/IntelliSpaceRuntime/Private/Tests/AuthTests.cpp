#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AuthSubsystem.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIS_Auth_OTPAndLogout, "IntelliSpace.Auth.OTPAndLogout",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIS_Auth_OTPAndLogout::RunTest(const FString& Parameters)
{
    UAuthSubsystem* Auth = NewObject<UAuthSubsystem>();
    const FString TestPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Test/auth_test.json"));
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(TestPath), true);
    Auth->__Test_SetJsonPath(TestPath);
    Auth->__Test_ClearProfile();

    Auth->StartOtp(TEXT("9999999999"), TEXT("Tester"));
    Auth->VerifyOtp(TEXT("123456"));
    TestTrue("LoggedIn after verify", Auth->IsLoggedIn());

    Auth->Logout();
    TestFalse("LoggedOut clears token", Auth->IsLoggedIn());
    return true;
}

#endif
