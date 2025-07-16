#pragma once

#include "CoreMinimal.h"
#include "NNETypes.h"
#include "PlatformNNERunner.h"
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#include "IOSNNERunner.generated.h"

UCLASS()
class INTELLISPACE_API UIOSNNERunner : public UPlatformNNERunner
{
    GENERATED_BODY()

public:
    virtual bool InitializeModel(const FString& ModelPath) override;
    
    virtual bool RunInference(const TArray<float>& InputTensor, int Width, int Height, TArray<float>& OutMaskTensor, TArray<TArray<float>>& OutputTensors, TArray<UE::NNE::FTensorBindingCPU>& OutputBindings) override;
    

private:
    TUniquePtr<Ort::Session> Session;
};


