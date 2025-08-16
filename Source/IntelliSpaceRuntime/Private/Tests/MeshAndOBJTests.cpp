// MeshAndOBJTests.cpp
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeshAndOBJTests, "IntelliSpace.MeshAndOBJ", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMeshAndOBJTests::RunTest(const FString& Parameters)
{
    const FString ObjP = FPaths::ProjectSavedDir() / TEXT("Tests") / TEXT("simple.obj");
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(ObjP), /*Tree*/true);

    const FString Text =
        TEXT("v 0 0 0\n")
        TEXT("v 1 0 0\n")
        TEXT("v 0 1 0\n")
        TEXT("f 1 2 3\n");

    TestTrue(TEXT("Save OBJ"), FFileHelper::SaveStringToFile(Text, *ObjP));
    return true;
}
