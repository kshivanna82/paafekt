#pragma once

#include "PlatformNNERunner.h"
#include "NNE.h"
#include "NNERuntime.h"
#include "NNERuntimeCPU.h"
#include "MacNNERunner.generated.h"

UCLASS()
class UMacNNERunner : public UPlatformNNERunner
{
    GENERATED_BODY()

public:
    virtual bool InitializeModel(const FString& ModelPath) override;
    virtual bool RunInference(const TArray<float>& InputTensor, int Width, int Height,
                              TArray<float>& OutMaskTensor, TArray<TArray<float>>& OutputTensors, TArray<UE::NNE::FTensorBindingCPU>& OutputBindings) override;

private:
    TWeakInterfacePtr<INNERuntimeCPU> CpuRuntime;
    TSharedPtr<UE::NNE::IModelCPU> CpuModel;
    TSharedPtr<UE::NNE::IModelInstanceCPU> CpuModelInstance;
};
