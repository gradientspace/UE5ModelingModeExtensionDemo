// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "SampleModelingModeExtensionCommands.h"
#include "EditorStyleSet.h"
#include "InputCoreTypes.h"
#include "SampleModelingModeExtensionStyle.h"

#define LOCTEXT_NAMESPACE "SampleModelingModeExtensionCommands"



FSampleModelingModeExtensionCommands::FSampleModelingModeExtensionCommands() :
	TCommands<FSampleModelingModeExtensionCommands>(
		"SampleModelingModeExtensionCommands", // Context name for fast lookup
		NSLOCTEXT("Contexts", "SampleModelingModeExtensionCommands", "Sample Modeling Mode Extension"), // Localized context name for displaying
		NAME_None, // Parent
		FSampleModelingModeExtensionStyle::Get()->GetStyleSetName() // Icon Style Set
		)
{
}


void FSampleModelingModeExtensionCommands::RegisterCommands()
{
	UI_COMMAND(BeginMeshNoiseTool, "Noise", "Add Noise to selected Mesh", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(BeginMeshPlaneCutTool, "Cut", "Cut the selected Mesh with a Plane", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(BeginActorClickedBPTool, "ClickBP", "Run BP on clicked Actors", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(BeginMeshProcessingBPTool, "MeshEdBP", "Run BP Mesh Processing Operation on selected Mesh", EUserInterfaceActionType::ToggleButton, FInputChord());
}




#undef LOCTEXT_NAMESPACE
