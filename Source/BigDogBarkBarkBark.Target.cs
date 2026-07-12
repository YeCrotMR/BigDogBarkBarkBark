// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class BigDogBarkBarkBarkTarget : TargetRules
{
	public BigDogBarkBarkBarkTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "BigDogBarkBarkBark" });
	}
}
