using UnrealBuildTool;
using System.Collections.Generic;

public class AfterclassEditorTarget : TargetRules
{
	public AfterclassEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
		ExtraModuleNames.AddRange(new string[] { "Afterclass", "AfterclassEditor" });
	}
}
