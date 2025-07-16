#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NNERuntimeCPU.h"
#include "PlatformNNERunner.generated.h"

UCLASS(Abstract)
class UPlatformNNERunner : public UObject
{
    GENERATED_BODY()

public:
    virtual bool InitializeModel(const FString& ModelPath) PURE_VIRTUAL(UPlatformNNERunner::InitializeModel, return false;);

    virtual bool RunInference(
        const TArray<float>& InputTensor,
        int Width,
        int Height,
        TArray<float>& OutMaskTensor,
        TArray<TArray<float>>& OutputTensors,
        TArray<UE::NNE::FTensorBindingCPU>& OutputBindings
    ) PURE_VIRTUAL(UPlatformNNERunner::RunInference, return false;);
};
