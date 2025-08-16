#include "OBJLoader.h"
#include "Misc/FileHelper.h"
#include "ISLog.h"

bool LoadOBJ_PositionsOnly(const FString& FilePath, TArray<FVector>& VOut, TArray<int32>& FOut)
{
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *FilePath)) {
        UE_LOG(LogIntelliSpace, Error, TEXT("[OBJ] Read failed: %s"), *FilePath);
        return false;
    }
    VOut.Reset(); FOut.Reset();
    TArray<FString> Lines; Text.ParseIntoArrayLines(Lines);

    TArray<FVector> Verts;
    Verts.Reserve(1000);
    auto ParseVec3 = [](const TArray<FString>& Toks)->FVector{
        double x=0,y=0,z=0;
        if (Toks.Num()>1) x = FCString::Atod(*Toks[1]);
        if (Toks.Num()>2) y = FCString::Atod(*Toks[2]);
        if (Toks.Num()>3) z = FCString::Atod(*Toks[3]);
        return FVector(x,y,z);
    };

    for (const FString& L : Lines)
    {
        if (L.StartsWith(TEXT("v "))) {
            TArray<FString> Toks; L.TrimStartAndEnd().ParseIntoArrayWS(Toks);
            if (Toks.Num() >= 4) Verts.Add(ParseVec3(Toks));
        }
        else if (L.StartsWith(TEXT("f "))) {
            TArray<FString> Toks; L.TrimStartAndEnd().ParseIntoArrayWS(Toks);
            if (Toks.Num() >= 4) {
                auto ParseIndex = [](const FString& S)->int32 {
                    FString A = S; int32 SlashPos;
                    if (A.FindChar('/', SlashPos)) A = A.Left(SlashPos);
                    return FMath::Max(FCString::Atoi(*A) - 1, 0);
                };
                int32 i0 = ParseIndex(Toks[1]);
                int32 i1 = ParseIndex(Toks[2]);
                int32 i2 = ParseIndex(Toks[3]);
                FOut.Add(i0); FOut.Add(i1); FOut.Add(i2);
            }
        }
    }
    if (Verts.Num() == 0 || FOut.Num() == 0) {
        UE_LOG(LogIntelliSpace, Warning, TEXT("[OBJ] No geometry in %s"), *FilePath);
        return false;
    }
    VOut = MoveTemp(Verts);
    UE_LOG(LogIntelliSpace, Log, TEXT("[OBJ] Loaded %d verts, %d tris from %s"), VOut.Num(), FOut.Num()/3, *FilePath);
    return true;
}
