//#if PLATFORM_IOS
#pragma once

#include "CoreMinimal.h"
#include "NNETypes.h"
#include "PlatformNNERunner.h"
//#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#include "IOSNNERunner.generated.h"

#if PLATFORM_IOS
namespace Ort { class Session; }
#endif

UCLASS()
class INTELLISPACE_API UIOSNNERunner : public UPlatformNNERunner
{
    GENERATED_BODY()

public:
//#if PLATFORM_IOS
    virtual bool InitializeModel(const FString& ModelPath) override;
    virtual bool RunInference(const TArray<float>& InputTensor, int Width, int Height, TArray<float>& OutMaskTensor, TArray<TArray<float>>& OutputTensors, TArray<UE::NNE::FTensorBindingCPU>& OutputBindings) override;
//#else
    // Stub fallback for Mac or other platforms
    //virtual bool InitializeModel(const FString&) override { return false; }
    //virtual bool RunInference(const TArray<float>&, int, int, TArray<float>&, TArray<TArray<float>>&, TArray<UE::NNE::FTensorBindingCPU>&) override { return false; }
//#endif
    
//#if PLATFORM_IOS
private:
#if PLATFORM_IOS
    //class Ort::Session;
    TUniquePtr<Ort::Session> Session;
    
#endif
};

//#endif

