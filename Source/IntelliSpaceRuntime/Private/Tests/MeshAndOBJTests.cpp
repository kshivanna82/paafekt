#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MeshGenerator.h"
#include "OBJLoader.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIS_Mesh_Build, "IntelliSpace.Mesh.Build",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIS_Mesh_Build::RunTest(const FString& Parameters)
{
    const int W=64, H=64;
    std::vector<float> D(W*H,0.f);
    for(int y=0;y<H;++y) for(int x=0;x<W;++x) D[y*W+x]=float(x+y)/float(W+H);
    TArray<FVector> V; TArray<int32> T;
    const bool ok = BuildMeshFromDepthMap(D,W,H,V,T,100.f,4);
    TestTrue("Mesh built", ok && V.Num()>0 && T.Num()>0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIS_OBJ_Load, "IntelliSpace.OBJ.Load",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIS_OBJ_Load::RunTest(const FString& Parameters)
{
    FString P = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Test/min.obj"));
    FString Text = TEXT("v 0 0 0
v 1 0 0
v 0 1 0
f 1 2 3
");
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(P), true);
    FFileHelper::SaveStringToFile(Text, *P);

    TArray<FVector> V; TArray<int32> T;
    const bool ok = LoadOBJ_PositionsOnly(P, V, T);
    TestTrue("OBJ loaded", ok && V.Num()==3 && T.Num()==3);
    return true;
}

#endif
