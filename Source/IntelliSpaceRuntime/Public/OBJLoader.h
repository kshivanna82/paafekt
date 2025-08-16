#pragma once
#include "CoreMinimal.h"
bool LoadOBJ_PositionsOnly(const FString& FilePath, TArray<FVector>& OutVertices, TArray<int32>& OutTriangles);
