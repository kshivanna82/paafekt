// IntelliSpaceRuntime.cpp
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// If other files in your project want to UE_LOG with LogIntelliSpaceRuntime,
// also add the small public header shown below and include it there.
DEFINE_LOG_CATEGORY_STATIC(LogIntelliSpaceRuntime, Log, All);

class FIntelliSpaceRuntimeModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogIntelliSpaceRuntime, Log, TEXT("IntelliSpaceRuntime module STARTUP"));
    }

    virtual void ShutdownModule() override
    {
        UE_LOG(LogIntelliSpaceRuntime, Log, TEXT("IntelliSpaceRuntime module SHUTDOWN"));
    }

    // This is a game module (not an editor-only module)
    virtual bool IsGameModule() const override { return true; }
};

IMPLEMENT_MODULE(FIntelliSpaceRuntimeModule, IntelliSpaceRuntime)
