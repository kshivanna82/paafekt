#include "SegDecorActor.h"
#include "CoreMinimal.h"
#include "PlatformCameraCaptureBridge.h"

ASegDecorActor::ASegDecorActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = false;
}

static void OnSegFrameThunk(void* User, FISPixelBufferRef Pixel)
{
    // For now just log; expand later as needed.
#if PLATFORM_IOS || PLATFORM_MAC
    if (Pixel)
    {
        CVPixelBufferLockBaseAddress(Pixel, kCVPixelBufferLock_ReadOnly);
        const size_t W = CVPixelBufferGetWidth(Pixel);
        const size_t H = CVPixelBufferGetHeight(Pixel);
        CVPixelBufferUnlockBaseAddress(Pixel, kCVPixelBufferLock_ReadOnly);
        UE_LOG(LogTemp, Verbose, TEXT("[SegDecor] Frame %dx%d"), (int)W, (int)H);
    }
#else
    UE_LOG(LogTemp, Verbose, TEXT("[SegDecor] Frame %p"), Pixel);
#endif
}

void ASegDecorActor::BeginPlay()
{
    Super::BeginPlay();
    RegisterCameraHandler(this, OnSegFrameThunk);
    StartCameraCapture();
}
