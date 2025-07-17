// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class IntelliSpaceTarget : TargetRules
{
	public IntelliSpaceTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		//ExtraModuleNames.Add("IntelliSpace");
        ExtraModuleNames.AddRange( new string[] { "IntelliSpace", "NNERuntimeORT" } );
	}
}
