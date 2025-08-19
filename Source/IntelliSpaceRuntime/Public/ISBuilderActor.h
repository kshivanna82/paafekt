#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ISBuilderActor.generated.h"

/**
 * Minimal implementation so UHT thunks resolve.
 */
UCLASS()
class INTELLISPACERUNTIME_API AISBuilderActor : public AActor
{
    GENERATED_BODY()

public:
    // Some build setups (and your generated code) referenced a no-arg ctor — provide both.
    AISBuilderActor();
    AISBuilderActor(const FObjectInitializer& ObjectInitializer);

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Review step handlers
    UFUNCTION(BlueprintCallable, Category="Builder|Review")
    void OnReview_Export();

    UFUNCTION(BlueprintCallable, Category="Builder|Review")
    void OnReview_Retake();

    UFUNCTION(BlueprintCallable, Category="Builder|Review")
    void OnReview_Discard();

    // Scan prompt handlers
    UFUNCTION(BlueprintCallable, Category="Builder|Scan")
    void OnScanPrompt_Reset();

    UFUNCTION(BlueprintCallable, Category="Builder|Scan")
    void OnScanPrompt_Discard();

    UFUNCTION(BlueprintCallable, Category="Builder|Scan")
    void OnScanPrompt_BuildNow();

    UFUNCTION(BlueprintCallable, Category="Builder|Scan")
    void OnScanPrompt_KeepScanning();

    // Other actions
    UFUNCTION(BlueprintCallable, Category="Builder")
    void DiscardCurrentCapture();
};
