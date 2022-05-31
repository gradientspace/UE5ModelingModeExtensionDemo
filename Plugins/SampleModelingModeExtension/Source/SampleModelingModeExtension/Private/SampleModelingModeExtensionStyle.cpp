// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "SampleModelingModeExtensionStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
#include "EditorStyleSet.h"
#include "Interfaces/IPluginManager.h"
#include "SlateOptMacros.h"
#include "Styling/SlateStyleMacros.h"

// IMAGE_BRUSH_SVG macro used below needs RootToContentDir to be defined?
#define RootToContentDir StyleSet->RootToContentDir

FString FSampleModelingModeExtensionStyle::InContent(const FString& RelativePath, const ANSICHAR* Extension)
{
	static FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("SampleModelingModeExtension"))->GetContentDir();
	return (ContentDir / RelativePath) + Extension;
}

TSharedPtr< FSlateStyleSet > FSampleModelingModeExtensionStyle::StyleSet = nullptr;
TSharedPtr< class ISlateStyle > FSampleModelingModeExtensionStyle::Get() { return StyleSet; }

FName FSampleModelingModeExtensionStyle::GetStyleSetName()
{
	static FName StyleName(TEXT("SampleModelingModeExtensionStyle"));
	return StyleName;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FSampleModelingModeExtensionStyle::Initialize()
{
	const FVector2D DefaultIconSize(20.0f, 20.0f);

	// Only register once
	if (StyleSet.IsValid())
	{
		return;
	}

	StyleSet = MakeShareable(new FSlateStyleSet(GetStyleSetName()));
	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("SampleModelingModeExtension"))->GetContentDir());
	StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	StyleSet->Set("SampleModelingModeExtensionCommands.BeginMeshNoiseTool", new IMAGE_BRUSH_SVG("Icons/NoiseTool", DefaultIconSize));
	StyleSet->Set("SampleModelingModeExtensionCommands.BeginMeshPlaneCutTool", new IMAGE_BRUSH_SVG("Icons/PlaneTool", DefaultIconSize));
	StyleSet->Set("SampleModelingModeExtensionCommands.BeginActorClickedBPTool", new IMAGE_BRUSH_SVG("Icons/ClickActorBPTool", DefaultIconSize));
	StyleSet->Set("SampleModelingModeExtensionCommands.BeginMeshProcessingBPTool", new IMAGE_BRUSH_SVG("Icons/MeshProcessingBPTool", DefaultIconSize));

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
};

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef DEFAULT_FONT

void FSampleModelingModeExtensionStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}
