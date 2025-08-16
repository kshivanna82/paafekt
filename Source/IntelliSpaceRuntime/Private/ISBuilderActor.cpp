// ISBuilderActor.cpp
#include "ISBuilderActor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

void AISBuilderActor::SaveCurrentMeshOBJ(const FString& Name)
{
    FString Out = TEXT("# MiDaS mesh\n");
    // Vertices
    for (const FVector& P : LastVerts)
    {
        Out += FString::Printf(TEXT("v %f %f %f\n"), P.X, P.Y, P.Z);
    }
    // Faces (OBJ is 1-indexed)
    for (int32 i = 0; i + 2 < LastTris.Num(); i += 3)
    {
        Out += FString::Printf(TEXT("f %d %d %d\n"),
                               LastTris[i] + 1, LastTris[i + 1] + 1, LastTris[i + 2] + 1);
    }

    const FString ObjDir = FPaths::ProjectSavedDir() / TEXT("Meshes");
    IFileManager::Get().MakeDirectory(*ObjDir, /*Tree*/true);
    const FString ObjP = ObjDir / (Name + TEXT(".obj"));
    FFileHelper::SaveStringToFile(Out, *ObjP);
}
