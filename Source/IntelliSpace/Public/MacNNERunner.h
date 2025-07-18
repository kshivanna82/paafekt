#pragma once


#include "PlatformNNERunner.h"
#if PLATFORM_MAC
#include "NNE.h"
#include "NNERuntime.h"
#include "NNERuntimeCPU.h"

#endif
#include "MacNNERunner.generated.h"

UCLASS()
class UMacNNERunner : public UPlatformNNERunner
{
    GENERATED_BODY()
#if PLATFORM_MAC
public:
    virtual bool InitializeModel(const FString& ModelPath) override;
    virtual bool RunInference(const TArray<float>& InputTensor, int Width, int Height,
                              TArray<float>& OutMaskTensor
    //TArray<TArray<float>>& OutputTensors,
    //TArray<UE::NNE::FTensorBindingCPU>& OutputBindings
    ) override;
    
private:
    TWeakInterfacePtr<INNERuntimeCPU> CpuRuntime;
    TSharedPtr<UE::NNE::IModelCPU> CpuModel;
    TSharedPtr<UE::NNE::IModelInstanceCPU> CpuModelInstance;
#endif
};

