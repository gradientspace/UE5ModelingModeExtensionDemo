// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "Tools/ActorClickedBPTool.h"
#include "ToolSceneQueriesUtil.h"



UInteractiveTool* UActorClickedBPToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	UActorClickedBPTool* NewTool = NewObject<UActorClickedBPTool>(SceneState.ToolManager);
	return NewTool;
}


void UActorClickedBPTool::Setup()
{
	USingleClickTool::Setup();

	PropertySet = NewObject<UActorClickedBPToolProperties>(this);
	AddToolPropertySource(PropertySet);
	PropertySet->RestoreProperties(this);
}

void UActorClickedBPTool::Shutdown(EToolShutdownType ShutdownType)
{
	PropertySet->SaveProperties(this);

	USingleClickTool::Shutdown(ShutdownType);
}


FInputRayHit UActorClickedBPTool::IsHitByClick(const FInputDeviceRay& ClickPos)
{
	// cast ray into scene
	FHitResult Result;
	if (ToolSceneQueriesUtil::FindNearestVisibleObjectHit(this, Result, ClickPos.WorldRay))
	{
		return FInputRayHit(Result.Distance);
	}
	return FInputRayHit();
}


void UActorClickedBPTool::OnClicked(const FInputDeviceRay& ClickPos)
{
	FHitResult Result;
	if (ToolSceneQueriesUtil::FindNearestVisibleObjectHit(this, Result, ClickPos.WorldRay))
	{
		AActor* Actor = Result.HitObjectHandle.FetchActor<AActor>();
		if (Actor != nullptr && PropertySet->Operation != nullptr)
		{
			UClass* ClassType = PropertySet->Operation;
			UActorClickedBPToolOperation* ClassObject = NewObject<UActorClickedBPToolOperation>((UObject*)GetTransientPackage(), ClassType);
			if (ClassObject)
			{
				ClassObject->OnApplyActionToActor(Actor);
			}
		}
	}
}