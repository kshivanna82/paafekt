// U2NetRunnerBridge.h
#pragma once
#include "CoreMinimal.h"
#include "PlatformCameraCaptureBridge.h" // <- gives FISPixelBufferRef

extern "C" {

// Example runner interface (adapt to your real signatures)
void* CreateU2NetRunner();
void  DestroyU2NetRunner(void* Runner);

// Run segmentation on a pixel buffer (Apple) or raw pointer (others) via FISPixelBufferRef
// Writes a single-channel mask [0..1] into OutMask (length = OutW*OutH).
bool  U2NetRun(void* Runner, FISPixelBufferRef Pixel, float* OutMask, int32 OutW, int32 OutH);

}
