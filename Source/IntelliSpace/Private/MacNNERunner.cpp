#include "MacNNERunner.h"

#if PLATFORM_MAC

#include "NNEModelData.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "NNERuntimeFormat.h"
#include "NNERuntimeCPU.h"

TArray<TArray<float>> OutputTensors;
TArray<UE::NNE::FTensorBindingCPU> OutputBindings;


bool UMacNNERunner::InitializeModel(const FString& ModelPath)
{
    TArray<uint8> RawData;
    if (!FFileHelper::LoadFileToArray(RawData, *ModelPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load ONNX model from %s"), *ModelPath);
        return false;
    }

    UNNEModelData* ModelData = NewObject<UNNEModelData>();
    ModelData->Init(TEXT("onnx"), RawData);

    CpuRuntime = UE::NNE::GetRuntime<INNERuntimeCPU>(TEXT("NNERuntimeORTCpu"));
    if (CpuRuntime.IsValid() && CpuRuntime->CanCreateModelCPU(ModelData) == INNERuntimeCPU::ECanCreateModelCPUStatus::Ok)
    {
        CpuModel = CpuRuntime->CreateModelCPU(ModelData);
        CpuModelInstance = CpuModel->CreateModelInstanceCPU();
    }
    return true;
}

bool UMacNNERunner::RunInference(const TArray<float>& InputTensor, int Width, int Height, TArray<float>& OutMaskTensor
                                 //TArray<TArray<float>>& OutputTensors,
                                 //TArray<UE::NNE::FTensorBindingCPU>& OutputBindings
                                 )
{
    if (!CpuModelInstance.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("CpuModelInstance is not valid."));
        return false;
    }
    
    TArray<uint32> InputDims = {1, 3, Height, Width};
    UE::NNE::FTensorShape InputShape = UE::NNE::FTensorShape::Make(InputDims);
    CpuModelInstance->SetInputTensorShapes({InputShape});
        
    
    CpuModelInstance->SetInputTensorShapes({ UE::NNE::FTensorShape::Make({1, 3, Height, Width}) });

    UE::NNE::FTensorBindingCPU InputBinding;
    InputBinding.Data = const_cast<float*>(InputTensor.GetData());
    InputBinding.SizeInBytes = InputTensor.Num() * sizeof(float);
    
    
    OutputTensors.Empty();
    OutputBindings.Empty();
    for (int i = 0; i < 7; ++i)
    {
        TArray<float> OutputTensor;
        OutputTensor.SetNum(Height * Width);  
        UE::NNE::FTensorBindingCPU Binding;
        Binding.Data = OutputTensor.GetData();
        Binding.SizeInBytes = OutputTensor.Num() * sizeof(float);
        OutputTensors.Add(MoveTemp(OutputTensor));
        OutputBindings.Add(Binding);
    }

    const UE::NNE::EResultStatus Status = CpuModelInstance->RunSync({InputBinding}, OutputBindings);

    if (Status != UE::NNE::EResultStatus::Ok)
    {
        UE_LOG(LogTemp, Error, TEXT("Model run failed with status %d"), static_cast<int32>(Status));
        return false;
    }

    return true;
}
#endif
