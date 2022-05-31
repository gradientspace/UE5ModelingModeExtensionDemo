using UnrealBuildTool;
using System.Collections.Generic;

public class UE5ToolPluginDemoTarget : TargetRules
{
	public UE5ToolPluginDemoTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange( new string[] { "UE5ToolPluginDemo" } );
	}
}
