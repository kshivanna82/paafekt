using UnrealBuildTool;
public class IntelliSpaceEditorTarget : TargetRules
{
    public IntelliSpaceEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        ExtraModuleNames.AddRange(new string[] { "IntelliSpaceRuntime" });
    }
}
