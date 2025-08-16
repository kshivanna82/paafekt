using UnrealBuildTool;
using System.IO;

public class IntelliSpaceRuntime : ModuleRules
{
    public IntelliSpaceRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core","CoreUObject","Engine","UMG","Slate","SlateCore",
            "Json","JsonUtilities","ProceduralMeshComponent","Projects","InteriorDesignPlugin"
        });

        bEnableObjCExceptions = true;

        if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            PublicFrameworks.AddRange(new string[] {
                "AVFoundation","CoreVideo","CoreMedia","CoreImage",
                "Accelerate","CoreML","Metal","UIKit","Foundation"
            });

            RuntimeDependencies.Add("$(ProjectDir)/Content/ML/MiDaS_swin2_tiny_256.mlmodelc/**", StagedFileType.NonUFS);
            RuntimeDependencies.Add("$(ProjectDir)/Content/ML/U2Net.mlmodelc/**", StagedFileType.NonUFS);
        }

        if (Target.Platform == UnrealTargetPlatform.Android)
        {
            bEnableExceptions = true;
            PublicAdditionalLibraries.Add("camera2ndk");
            PublicAdditionalLibraries.Add("mediandk");
            PublicAdditionalLibraries.Add("log");
        }
    }
}
