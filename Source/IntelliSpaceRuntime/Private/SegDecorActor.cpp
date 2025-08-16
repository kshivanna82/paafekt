// SegDecorActor.cpp (fixed)
#include "SegDecorActor.h"
#include "CoreMinimal.h"

// Uniform callback signature
using FSegFrameCB = void(*)(void*, void*);

// Provided by platform capture (declared here to avoid include order issues)
extern "C" {
    void RegisterCameraHandler(void* UserData, FSegFrameCB Callback);
    void StartCameraCapture();
}

static void OnSegFrameThunk(void* User, void* Pixel)
{
    ASegDecorActor* Self = static_cast<ASegDecorActor*>(User);
    if (!Self) return;
    Self->OnSegFrame(Pixel);
}

ASegDecorActor::ASegDecorActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ASegDecorActor::BeginPlay()
{
    Super::BeginPlay();
    RegisterCameraHandler(this, &OnSegFrameThunk);
    StartCameraCapture();
}

void ASegDecorActor::OnSegFrame(void* PixelBuffer)
{
#if PLATFORM_IOS || PLATFORM_MAC
    CVPixelBufferRef PB = (CVPixelBufferRef)PixelBuffer;
    if (!PB) return;
    CVPixelBufferLockBaseAddress(PB, kCVPixelBufferLock_ReadOnly);
    void* Base = CVPixelBufferGetBaseAddress(PB);
    const size_t W = CVPixelBufferGetWidth(PB);
    const size_t H = CVPixelBufferGetHeight(PB);
    const size_t Row = CVPixelBufferGetBytesPerRow(PB);
    UE_LOG(LogTemp, Verbose, TEXT("[SegDecor] Frame %dx%d row=%d base=%p"), (int)W, (int)H, (int)Row, Base);
    CVPixelBufferUnlockBaseAddress(PB, kCVPixelBufferLock_ReadOnly);
#else
    UE_LOG(LogTemp, Verbose, TEXT("[SegDecor] Non-Apple frame ptr=%p"), PixelBuffer);
#endif
}
