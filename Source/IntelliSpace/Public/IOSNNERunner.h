#pragma once

#include "CoreMinimal.h"
// Any other headers you need here



#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#include "IOSNNERunner.generated.h"

UCLASS()
class INTELLISPACE_API UIOSNNERunner : public UObject
{
    GENERATED_BODY()

public:
    bool InitializeModel(const FString& ModelPath);
    bool RunInference(const TArray<float>& InputTensor, int Width, int Height, TArray<float>& OutMaskTensor);

private:
    TUniquePtr<Ort::Session> Session;
};
