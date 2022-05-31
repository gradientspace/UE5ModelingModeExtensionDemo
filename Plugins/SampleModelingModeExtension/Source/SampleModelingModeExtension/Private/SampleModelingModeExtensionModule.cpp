// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "SampleModelingModeExtensionModule.h"
#include "SampleModelingModeExtensionStyle.h"
#include "SampleModelingModeExtensionCommands.h"

#include "Tools/MeshNoiseTool.h"
#include "Tools/MeshPlaneCutTool.h"
#include "Tools/ActorClickedBPTool.h"
#include "Tools/MeshProcessingBPTool.h"

#define LOCTEXT_NAMESPACE "FSampleModelingModeExtensionModule"



// IModuleInterface API implementation

void FSampleModelingModeExtensionModule::StartupModule()
{
	FSampleModelingModeExtensionStyle::Initialize();
	FSampleModelingModeExtensionCommands::Register();

	IModularFeatures::Get().RegisterModularFeature(IModelingModeToolExtension::GetModularFeatureName(), this);
}

void FSampleModelingModeExtensionModule::ShutdownModule()
{
	IModularFeatures::Get().UnregisterModularFeature(IModelingModeToolExtension::GetModularFeatureName(), this);

	FSampleModelingModeExtensionCommands::Unregister();
	FSampleModelingModeExtensionStyle::Shutdown();
}



// IModelingModeToolExtension API implementation

FText FSampleModelingModeExtensionModule::GetExtensionName()
{
	return LOCTEXT("ExtensionName", "Sample Modeling Extension");
}

FText FSampleModelingModeExtensionModule::GetToolSectionName()
{
	return LOCTEXT("SectionName", "Extension Demo");
}

void FSampleModelingModeExtensionModule::GetExtensionTools(const FExtensionToolQueryInfo& QueryInfo, TArray<FExtensionToolDescription>& ToolsOut)
{
	FExtensionToolDescription MeshNoiseToolInfo;
	MeshNoiseToolInfo.ToolName = LOCTEXT("MeshNoiseTool", "MeshNoise");
	MeshNoiseToolInfo.ToolCommand = FSampleModelingModeExtensionCommands::Get().BeginMeshNoiseTool;
	MeshNoiseToolInfo.ToolBuilder = NewObject<UMeshNoiseToolBuilder>();
	ToolsOut.Add(MeshNoiseToolInfo);

	FExtensionToolDescription MeshPlaneCutToolInfo;
	MeshPlaneCutToolInfo.ToolName = LOCTEXT("MeshPlaneCut", "PlaneCut");
	MeshPlaneCutToolInfo.ToolCommand = FSampleModelingModeExtensionCommands::Get().BeginMeshPlaneCutTool;
	MeshPlaneCutToolInfo.ToolBuilder = NewObject<UMeshPlaneCutToolBuilder>();
	ToolsOut.Add(MeshPlaneCutToolInfo);

	FExtensionToolDescription BPActionToolInfo;
	BPActionToolInfo.ToolName = LOCTEXT("ActorClickedBPTool", "ActorClickedBP");
	BPActionToolInfo.ToolCommand = FSampleModelingModeExtensionCommands::Get().BeginActorClickedBPTool;
	BPActionToolInfo.ToolBuilder = NewObject<UActorClickedBPToolBuilder>();
	ToolsOut.Add(BPActionToolInfo);

	FExtensionToolDescription MeshProcessingBPToolInfo;
	MeshProcessingBPToolInfo.ToolName = LOCTEXT("MeshProcessingBPTool", "MeshProcessingBP");
	MeshProcessingBPToolInfo.ToolCommand = FSampleModelingModeExtensionCommands::Get().BeginMeshProcessingBPTool;
	MeshProcessingBPToolInfo.ToolBuilder = NewObject<UMeshProcessingBPToolBuilder>();
	ToolsOut.Add(MeshProcessingBPToolInfo);

}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSampleModelingModeExtensionModule, SampleModelingModeExtension)