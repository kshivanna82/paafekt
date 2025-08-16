// PlatformCameraCaptureBridge.h
#pragma once

#include "CoreMinimal.h"

// --- Unify pixel-buffer type across the whole project -------------------------
// On Apple platforms, use CoreVideo's CVPixelBufferRef.
// Elsewhere, alias to void* so signatures are consistent without redefining CVPixelBufferRef.
#if PLATFORM_IOS || PLATFORM_MAC
    #include <CoreVideo/CoreVideo.h>
    using FISPixelBufferRef = CVPixelBufferRef;
#else
    using FISPixelBufferRef = void*;
#endif

// Camera frame callback signature used by all modules (iOS, Android stubs, tests, etc.)
extern "C" {
    typedef void (*FISCameraFrameCallback)(void* UserData, FISPixelBufferRef PixelBuffer);

    // Register a single global handler (simple bridge model). Calling again overwrites.
    void RegisterCameraHandler(void* UserData, FISCameraFrameCallback Callback);

    // Start/stop capture on the active platform. No-ops on platforms without impls.
    void StartCameraCapture();
    void StopCameraCapture();
}

// Minimal inline default (no-op) impls for non-Apple platforms to avoid link errors.
// The iOS .mm will provide real implementations. You can also provide Android ones later.
#if !(PLATFORM_IOS)
inline void RegisterCameraHandler(void* /*UserData*/, FISCameraFrameCallback /*Callback*/) {}
inline void StartCameraCapture() {}
inline void StopCameraCapture() {}
#endif
