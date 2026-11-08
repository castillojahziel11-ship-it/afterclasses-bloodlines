using UnrealBuildTool;

public class AfterclassEditor : ModuleRules
{
	public AfterclassEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "Slate", "SlateCore", "Afterclass" });
	}
}
