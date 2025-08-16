#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ModelRepositorySubsystem.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIS_Repo_SaveLoadDelete, "IntelliSpace.Repo.SaveLoadDelete",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIS_Repo_SaveLoadDelete::RunTest(const FString& Parameters)
{
    UModelRepositorySubsystem* Repo = NewObject<UModelRepositorySubsystem>();
    const FString TestDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Test"));
    const FString TestPath = FPaths::Combine(TestDir, TEXT("models_test.json"));
    IFileManager::Get().MakeDirectory(*TestDir, true);
    Repo->__Test_SetJsonPath(TestPath);

    const FString ObjP = FPaths::Combine(TestDir, TEXT("foo.obj"));
    FFileHelper::SaveStringToFile(TEXT("v 0 0 0
"), *ObjP);

    FISModelEntry E; E.Name=TEXT("Foo"); E.ObjPath=ObjP; E.ThumbnailPath=TEXT("");
    Repo->AddModel(E);
    Repo->Load();

    const bool removed = Repo->RemoveModelByName(TEXT("Foo"), true);
    TestTrue("Removed entry by name", removed);
    TestTrue("OBJ deleted", !FPaths::FileExists(ObjP));
    return true;
}

#endif
