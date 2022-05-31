// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class SAMPLEMODELINGMODEEXTENSION_API FSampleModelingModeExtensionCommands : public TCommands<FSampleModelingModeExtensionCommands>
{
public:
	FSampleModelingModeExtensionCommands();

	TSharedPtr<FUICommandInfo> BeginMeshNoiseTool;
	TSharedPtr<FUICommandInfo> BeginMeshPlaneCutTool;
	TSharedPtr<FUICommandInfo> BeginActorClickedBPTool;
	TSharedPtr<FUICommandInfo> BeginMeshProcessingBPTool;

	/**
	 * Initialize commands
	 */
	virtual void RegisterCommands() override;
};
