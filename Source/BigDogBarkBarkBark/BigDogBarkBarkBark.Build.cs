// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BigDogBarkBarkBark : ModuleRules
{
	public BigDogBarkBarkBark(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[]
		{
			ModuleDirectory,
			System.IO.Path.Combine(ModuleDirectory, "Core"),
			System.IO.Path.Combine(ModuleDirectory, "Lane"),
			System.IO.Path.Combine(ModuleDirectory, "Unit"),
			System.IO.Path.Combine(ModuleDirectory, "Building"),
			System.IO.Path.Combine(ModuleDirectory, "Wave"),
			System.IO.Path.Combine(ModuleDirectory, "UI")
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UMG",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
