// PlatformCameraCaptureBridge.h
#pragma once

#include "CoreMinimal.h"

#if PLATFORM_IOS || PLATFORM_MAC
    #include <CoreVideo/CoreVideo.h>
    using FISPixelBufferRef = CVPixelBufferRef;
#else
    using FISPixelBufferRef = void*;
#endif

extern "C" {
    typedef void (*FISCameraFrameCallback)(void* UserData, FISPixelBufferRef PixelBuffer);
    void RegisterCameraHandler(void* UserData, FISCameraFrameCallback Callback);
    void StartCameraCapture();
    void StopCameraCapture();
}

#if !(PLATFORM_IOS)
inline void RegisterCameraHandler(void* /*UserData*/, FISCameraFrameCallback /*Callback*/) {}
inline void StartCameraCapture() {}
inline void StopCameraCapture() {}
#endif
