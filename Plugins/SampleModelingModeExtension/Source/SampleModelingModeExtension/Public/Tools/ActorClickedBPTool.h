// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "CoreMinimal.h"
#include "BaseTools/SingleClickTool.h"
#include "GameFramework/Actor.h"
#include "ActorClickedBPTool.generated.h"

/**
 * UActorClickedBPToolOperation is intended to be used as a basis for Blueprints
 * that can process an Actor, which can then be configured as the BP operation in
 * a UActorClickedBPTool instance.
 */
UCLASS(BlueprintType, Blueprintable)
class SAMPLEMODELINGMODEEXTENSION_API UActorClickedBPToolOperation : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintImplementableEvent, CallInEditor, Category = "Events")
	void OnApplyActionToActor(AActor* TargetActor);
};



/**
 * Tool Settings for a UActorClickedBPTool
 */
UCLASS()
class SAMPLEMODELINGMODEEXTENSION_API UActorClickedBPToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	/** Blueprint to execute on Actor click */
	UPROPERTY(EditAnywhere, Category = Options)
	TSubclassOf<UActorClickedBPToolOperation> Operation;
};


/**
 * UActorClickedBPTool is a Tool that executes an arbitrary Blueprint when an Actor in the active
 * level is clicked with the cursor. The Blueprint must be a subclass of UActorClickedBPToolOperation,
 * and is configured via the UActorClickedBPToolProperties.
 */
UCLASS()
class SAMPLEMODELINGMODEEXTENSION_API UActorClickedBPTool : public USingleClickTool
{
	GENERATED_BODY()

public:
	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;

	virtual FInputRayHit IsHitByClick(const FInputDeviceRay& ClickPos) override;

	virtual void OnClicked(const FInputDeviceRay& ClickPos) override;

public:
	UPROPERTY()
	TObjectPtr<UActorClickedBPToolProperties> PropertySet;
};



UCLASS()
class SAMPLEMODELINGMODEEXTENSION_API UActorClickedBPToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()
public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override { return true; }
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};