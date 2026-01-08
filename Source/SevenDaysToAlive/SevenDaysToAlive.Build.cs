// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SevenDaysToAlive : ModuleRules
{
	public SevenDaysToAlive(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"SevenDaysToAlive",
			"SevenDaysToAlive/Variant_Horror",
			"SevenDaysToAlive/Variant_Horror/UI",
			"SevenDaysToAlive/Variant_Shooter",
			"SevenDaysToAlive/Variant_Shooter/AI",
			"SevenDaysToAlive/Variant_Shooter/UI",
			"SevenDaysToAlive/Variant_Shooter/Weapons",
			"SevenDaysToAlive/Variant_Survival",
			"SevenDaysToAlive/Variant_Survival/Core",
			"SevenDaysToAlive/Variant_Survival/Core/Game",
			"SevenDaysToAlive/Variant_Survival/Enemies/AI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
