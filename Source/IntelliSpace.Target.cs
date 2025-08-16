using UnrealBuildTool;
public class IntelliSpaceTarget : TargetRules
{
    public IntelliSpaceTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        ExtraModuleNames.AddRange(new string[] { "IntelliSpaceRuntime" });
    }
}
