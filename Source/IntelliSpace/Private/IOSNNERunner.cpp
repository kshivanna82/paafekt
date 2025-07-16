#include "IOSNNERunner.h"

#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"

#if PLATFORM_IOS

#include <onnxruntime/core/session/onnxruntime_cxx_api.h>

bool UIOSNNERunner::InitializeModel(const FString& ModelPath)
{
    static Ort::Env OrtEnv(ORT_LOGGING_LEVEL_WARNING, "UnrealORT"); 
    Ort::SessionOptions SessionOptions;
    FString AbsolutePath = FPaths::ConvertRelativePathToFull(ModelPath);
    Session = MakeUnique<Ort::Session>(OrtEnv, TCHAR_TO_UTF8(*AbsolutePath), SessionOptions);
    return true;
}

bool UIOSNNERunner::RunInference(const TArray<float>& InputTensor, int Width, int Height, TArray<float>& OutMaskTensor, TArray<TArray<float>>& OutputTensors,
                                 TArray<UE::NNE::FTensorBindingCPU>& OutputBindings)
{
    const int64 InputDims[] = {1, 3, Height, Width};
    Ort::MemoryInfo MemInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);

    Ort::Value Input = Ort::Value::CreateTensor<float>(MemInfo, const_cast<float*>(InputTensor.GetData()), InputTensor.Num(), InputDims, 4);

    auto OutputNames = Session->GetOutputNameAllocated(0, Ort::AllocatorWithDefaultOptions());
    std::array<const char*, 1> OutputNamesArray = {OutputNames.get()};

    auto OutputTensors = Session->Run(Ort::RunOptions{nullptr}, nullptr, &Input, 1, OutputNamesArray.data(), 1);
    float* OutputData = OutputTensors[0].GetTensorMutableData<float>();

    OutMaskTensor.SetNum(Width * Height);
    FMemory::Memcpy(OutMaskTensor.GetData(), OutputData, OutMaskTensor.Num() * sizeof(float));
    return true;
}

#else

// Stub fallback for Mac to satisfy linker

bool UIOSNNERunner::InitializeModel(const FString&)
{
    return false;
}

bool UIOSNNERunner::RunInference(const TArray<float>&, int, int,
                                 TArray<float>&,
                                 TArray<TArray<float>>&,
                                 TArray<UE::NNE::FTensorBindingCPU>&)
{
    // Dummy implementation
    return false;
}

#endif

