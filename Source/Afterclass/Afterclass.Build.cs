using System;
using UnrealBuildTool;

public class Afterclass : ModuleRules
{
	public Afterclass(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Allow "Core/...", "Combat/...", "Characters/..." style includes from any file in (or depending on) this module.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"GameplayTags", "GameplayTasks", "GameplayAbilities",
			"AIModule", "UMG", "CommonUI", "CommonInput",
			"OnlineSubsystem", "NetCore", "DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Build number is injected by CI via the AC_BUILD_NUMBER environment variable (defaults to 0 for local builds).
		int BuildNumber = 0;
		int.TryParse(Environment.GetEnvironmentVariable("AC_BUILD_NUMBER") ?? "0", out BuildNumber);
		PublicDefinitions.Add("AC_BUILD_NUMBER=" + BuildNumber);
	}
}
