#pragma once
#include "CoreMinimal.h"
#include <vector>

bool BuildMeshFromDepthMap(const std::vector<float>& Depth, int W, int H,
                           TArray<FVector>& OutVertices, TArray<int32>& OutTriangles,
                           float DepthScale = 100.0f, int Step = 2);
