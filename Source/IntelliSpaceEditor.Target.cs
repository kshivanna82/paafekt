using UnrealBuildTool;
public class IntelliSpaceEditorTarget : TargetRules
{
    public IntelliSpaceEditorTarget( TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
        //ExtraModuleNames.Add("IntelliSpace", "NNERuntimeORT");
        ExtraModuleNames.AddRange( new string[] { "IntelliSpace", "NNERuntimeORT" } );
    }
}
