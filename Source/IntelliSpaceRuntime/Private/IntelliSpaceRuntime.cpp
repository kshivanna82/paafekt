#include "IntelliSpaceRuntime.h" // must be first

#include "ISLog.h"

void FIntelliSpaceRuntimeModule::StartupModule()
{
    UE_LOG(LogIntelliSpace, Log, TEXT("IntelliSpaceRuntime module started"));
}

void FIntelliSpaceRuntimeModule::ShutdownModule()
{
    UE_LOG(LogIntelliSpace, Log, TEXT("IntelliSpaceRuntime module shutdown"));
}

IMPLEMENT_MODULE(FIntelliSpaceRuntimeModule, IntelliSpaceRuntime)
