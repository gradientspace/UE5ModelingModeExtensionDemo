// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "CoreMinimal.h"
#include "BaseTools/BaseMeshProcessingTool.h"
#include "MeshProcessingBPTool.generated.h"

class FBackgroundMeshProcessingExecutor;

/**
 * Generic set of parameters for a UMeshProcessingBPToolOperation.
 * Add additional parameters here to expose in the Tool Settings.
 */
USTRUCT(BlueprintType)
struct SAMPLEMODELINGMODEEXTENSION_API FMeshProcessingBPToolParameters
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Parameters)
	float FloatParam1 = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Parameters)
	float FloatParam2 = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Parameters)
	int IntParam1 = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Parameters)
	int IntParam2 = 0;
};



/**
 * UMeshProcessingBPToolOperation is intended to be used as a basis for Blueprints
 * that can process a mesh, which can then be configured as the BP operation in
 * a UMeshProcessingBPTool instance.
 */
UCLASS(Blueprintable, BlueprintType)
class SAMPLEMODELINGMODEEXTENSION_API UMeshProcessingBPToolOperation : public UObject
{
	GENERATED_BODY()
public:
	/**
	 * Implement this function to apply a mesh processing operation to the TargetMesh
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnRecomputeMesh(UDynamicMesh* TargetMesh, FMeshProcessingBPToolParameters Parameters);

	/**
	 * Override GetEnableBackgroundExecution in BP to indicate that it is acceptable
	 * to execute OnRecomputeMesh from a background thread. This is only the case if
	 * no calls to external UObjects or Subsystems are used, ie only basic Geometry Script 
	 * operations are used with the TargetMesh.
	 * 
	 * (It's probably not a good idea to use this, it may stop working in the future!)
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Events")
	bool GetEnableBackgroundExecution();
	bool GetEnableBackgroundExecution_Implementation();
};



/**
 * Tool Settings for a UMeshProcessingBPTool
 */
UCLASS()
class SAMPLEMODELINGMODEEXTENSION_API UMeshProcessingBPToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	/** Blueprint to execute */
	UPROPERTY(EditAnywhere, Category = Operation)
	TSubclassOf<UMeshProcessingBPToolOperation> Operation;

	UPROPERTY(EditAnywhere, Category = Settings)
	FMeshProcessingBPToolParameters Parameters;
};



/**
 * UMeshProcessingBPTool is a Mesh Processing Tool that executes an arbitrary Blueprint to do
 * the mesh processing operation. The Blueprint must be a subclass of UMeshProcessingBPToolOperation,
 * and is configured via the UMeshProcessingBPToolProperties
 */
UCLASS()
class SAMPLEMODELINGMODEEXTENSION_API UMeshProcessingBPTool : public UBaseMeshProcessingTool
{
	GENERATED_BODY()

public:
	UMeshProcessingBPTool();

protected:
	// UBaseMeshProcessingTool API implementation

	virtual void InitializeProperties() override;
	virtual void OnShutdown(EToolShutdownType ShutdownType) override;
	virtual void OnTick(float DeltaTime) override;

	virtual TUniquePtr<UE::Geometry::FDynamicMeshOperator> MakeNewOperator() override;

	virtual bool RequiresInitialVtxNormals() const { return false; }
	virtual bool HasMeshTopologyChanged() const override;

	virtual FText GetToolMessageString() const override;
	virtual FText GetAcceptTransactionName() const override;

	// disable scaling to unit dimensions, this is a feature of UBaseMeshProcessingTool that is enabled by default
	virtual bool RequiresScaleNormalization() const { return false; }

protected:
	// settings for this Tool that will be exposed in Modeling Mode details panel
	UPROPERTY()
	TObjectPtr<UMeshProcessingBPToolProperties> Properties = nullptr;

	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;

	// A helper class (defined in cpp) that is used to force execution of the Blueprint operation on the game thread
	TSharedPtr<FBackgroundMeshProcessingExecutor> Executor;
};




UCLASS()
class SAMPLEMODELINGMODEEXTENSION_API UMeshProcessingBPToolBuilder : public UBaseMeshProcessingToolBuilder
{
	GENERATED_BODY()
public:
	virtual UBaseMeshProcessingTool* MakeNewToolInstance(UObject* Outer) const 
	{
		return NewObject<UMeshProcessingBPTool>(Outer);
	}
};
