// MiDaSRunnerBridge.h
#pragma once
#include "CoreMinimal.h"
#include "PlatformCameraCaptureBridge.h" // <- FISPixelBufferRef

extern "C" {

void* CreateMiDaSRunner();
void  DestroyMiDaSRunner(void* Runner);

// Produces a depth map into OutDepth (W*H floats). Return false on failure.
bool  MiDaSRun(void* Runner, FISPixelBufferRef Pixel, float* OutDepth, int32 OutW, int32 OutH);

}
