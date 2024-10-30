// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class Chasing_5SD073Target : TargetRules
{
	public Chasing_5SD073Target( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		bOverrideBuildEnvironment = true;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_1;
		ExtraModuleNames.Add("Chasing_5SD073");
	}
}
