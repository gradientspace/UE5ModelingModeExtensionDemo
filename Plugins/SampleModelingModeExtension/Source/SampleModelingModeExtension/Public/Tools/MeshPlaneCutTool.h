// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "CoreMinimal.h"
#include "BaseTools/BaseMeshProcessingTool.h"
#include "MeshPlaneCutTool.generated.h"

class UCombinedTransformGizmo;
class UTransformProxy;


UCLASS()
class SAMPLEMODELINGMODEEXTENSION_API UMeshPlaneCutProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = Plane, meta = (TransientToolProperty))
	bool bFillHole = true;

	UPROPERTY(EditAnywhere, Category = Plane, meta = (TransientToolProperty))
	FVector Position;

	UPROPERTY(EditAnywhere, Category = Plane, meta = (TransientToolProperty))
	FRotator Rotation;
};



/**
 * UMeshPlaneCutTool apples a Plane Cut to a mesh, deleting one side of the cut and
 * filling any holes created by the cut. A 3D Gizmo used to provide a user interface for
 * positioning the plane.
 */
UCLASS()
class SAMPLEMODELINGMODEEXTENSION_API UMeshPlaneCutTool : public UBaseMeshProcessingTool
{
	GENERATED_BODY()

public:
	UMeshPlaneCutTool();

protected:
	// UBaseMeshProcessingTool API implementation

	virtual void InitializeProperties() override;
	virtual void OnShutdown(EToolShutdownType ShutdownType) override;

	virtual TUniquePtr<UE::Geometry::FDynamicMeshOperator> MakeNewOperator() override;

	// Do not need vertex normals for this UBaseMeshProcessingTool
	virtual bool RequiresInitialVtxNormals() const { return false; }
	// If we change the mesh this must return true. Depends on what the Tool does...
	virtual bool HasMeshTopologyChanged() const override;

	virtual FText GetToolMessageString() const override;
	virtual FText GetAcceptTransactionName() const override;

	// disable scaling to unit dimensions, this is a feature of UBaseMeshProcessingTool that is enabled by default
	virtual bool RequiresScaleNormalization() const { return false; }

protected:
	//  settings for this Tool that will be exposed in Modeling Mode details panel
	UPROPERTY()
	TObjectPtr<UMeshPlaneCutProperties> Properties = nullptr;

protected:
	// Transform of the 3D plane is tracked independently of the exposed Properties, this
	// is necessary to allow the Gizmo and PropertySet to both update the plane
	FTransform3d PlaneTransform;


protected:
	// 3D Transform Gizmo support, used to interactively position the 3D plane

	UPROPERTY()
	TObjectPtr<UCombinedTransformGizmo> PlaneTransformGizmo;
	
	UPROPERTY()
	TObjectPtr<UTransformProxy> PlaneTransformProxy;

	void InitializeTransformGizmo();
	void OnGizmoTransformChanged(UTransformProxy* Proxy, FTransform Transform);

};




UCLASS()
class SAMPLEMODELINGMODEEXTENSION_API UMeshPlaneCutToolBuilder : public UBaseMeshProcessingToolBuilder
{
	GENERATED_BODY()
public:
	virtual UBaseMeshProcessingTool* MakeNewToolInstance(UObject* Outer) const 
	{
		return NewObject<UMeshPlaneCutTool>(Outer);
	}
};
