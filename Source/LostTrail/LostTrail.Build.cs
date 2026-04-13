// Copyright Kriisko-Studio. Licensed under project terms.

using UnrealBuildTool;

public class LostTrail : ModuleRules
{
	public LostTrail(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"HTTP",
			"Json",
			"JsonUtilities",
			"UMG",
			"Slate",
			"SlateCore",
			"AIModule",
			"NavigationSystem",
			"GameplayTags"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"TP_ThirdPerson"
		});

		// Module root contains LostTrail.h (log category). Subdirectories match
		// the same layout as TP_ThirdPerson's explicit include paths.
		PublicIncludePaths.AddRange(new string[]
		{
			"LostTrail",
			"LostTrail/Public",
			"LostTrail/Public/Dog",
			"LostTrail/Public/Survival",
			"LostTrail/Public/Translator",
			"LostTrail/Public/World"
		});
	}
}
