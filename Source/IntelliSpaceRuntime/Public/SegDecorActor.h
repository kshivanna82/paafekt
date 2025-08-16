// SegDecorActor.h (fixed)
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SegDecorActor.generated.h"

// Forward declare to avoid heavy includes here
struct __CVBuffer;
typedef __CVBuffer* CVPixelBufferRef;
class ASegDecorActor;

UCLASS(BlueprintType)
class INTELLISPACERUNTIME_API ASegDecorActor : public AActor
{
    GENERATED_BODY()
public:
    ASegDecorActor();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override {}

    // Use CVPixelBufferRef on Apple, opaque elsewhere
    void OnSegFrame(void* PixelBuffer);
};
