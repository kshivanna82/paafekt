using UnrealBuildTool;
public class IntelliSpaceTarget : TargetRules
{
    public IntelliSpaceTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        ExtraModuleNames.AddRange(new string[] { "IntelliSpace" });
        
        if (Target.Platform == UnrealTargetPlatform.IOS || Target.Platform == UnrealTargetPlatform.Android)
        {
            // Needed for GInternalProjectName, GIsGameAgnosticExe, etc.
            ExtraModuleNames.Add("Launch");
        }
    }
}
