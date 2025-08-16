
using UnrealBuildTool;
public class InteriorDesignPlugin : ModuleRules
{
    public InteriorDesignPlugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core","CoreUObject","Engine","UMG","Slate","SlateCore","ProceduralMeshComponent","Json","JsonUtilities"
        });
    }
}
