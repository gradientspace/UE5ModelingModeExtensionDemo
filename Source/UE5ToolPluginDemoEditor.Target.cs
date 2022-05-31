using UnrealBuildTool;
using System.Collections.Generic;

public class UE5ToolPluginDemoEditorTarget : TargetRules
{
	public UE5ToolPluginDemoEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange( new string[] { "UE5ToolPluginDemo" } );
	}
}
