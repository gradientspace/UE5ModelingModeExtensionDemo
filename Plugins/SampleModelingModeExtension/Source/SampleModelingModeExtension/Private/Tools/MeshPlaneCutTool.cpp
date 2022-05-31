// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "Tools/MeshPlaneCutTool.h"
#include "InteractiveToolManager.h"
#include "InteractiveGizmoManager.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/MeshNormals.h"
#include "DynamicMesh/MeshTransforms.h"
#include "ModelingOperators.h"
#include "BaseGizmos/TransformGizmoUtil.h"

#include "Operations/MeshPlaneCut.h"
#include "ConstrainedDelaunay2.h"

using namespace UE::Geometry;

#define LOCTEXT_NAMESPACE "UMeshPlaneCutTool"




UMeshPlaneCutTool::UMeshPlaneCutTool()
{
	SetToolDisplayName(LOCTEXT("ToolName", "Plane Cut"));
}



void UMeshPlaneCutTool::InitializeProperties()
{
	// create the tool options property set
	Properties = NewObject<UMeshPlaneCutProperties>(this);
	AddToolPropertySource(Properties);
	Properties->RestoreProperties(this);

	// initialize the plane to be at the center of the target object bounding box, in world space
	FAxisAlignedBox3d Bounds = GetInitialMesh().GetBounds();
	FVector3d WorldSpaceOrigin = GetPreviewTransform().TransformPosition(Bounds.Center());
	PlaneTransform = FTransform3d(WorldSpaceOrigin);

	// Initialize the Position and Rotation tool properties. This should be done before
	// setting up the Gizmo to avoid triggering an initial change event
	Properties->Position = PlaneTransform.GetTranslation();
	Properties->Rotation = PlaneTransform.Rotator();

	// create and configure the Gizmo
	InitializeTransformGizmo();

	// Watch for changes to the Position and Rotation properties, and update the Transform & Gizmo accordingly
	Properties->WatchProperty(Properties->Position, [this](const FVector& NewPosition)
	{ 
		PlaneTransform.SetTranslation(NewPosition);
		PlaneTransformGizmo->SetNewGizmoTransform(PlaneTransform);
		InvalidateResult();
	});
	Properties->WatchProperty(Properties->Rotation, [this](const FRotator& NewRotation)
	{
		PlaneTransform = FTransform3d(NewRotation, PlaneTransform.GetTranslation());
		PlaneTransformGizmo->SetNewGizmoTransform(PlaneTransform);
		InvalidateResult();
	});
}


void UMeshPlaneCutTool::InitializeTransformGizmo()
{
	// create a "transform proxy" for the Gizmo to manipulate
	PlaneTransformProxy = NewObject<UTransformProxy>(this);
	// now create a Gizmo
	PlaneTransformGizmo = UE::TransformGizmoUtil::CreateCustomTransformGizmo(GetToolManager(),
		ETransformGizmoSubElements::StandardTranslateRotate, this);
	// listen for changes to the TransformProxy caused by the Gizmo
	PlaneTransformProxy->OnTransformChanged.AddUObject(this, &UMeshPlaneCutTool::OnGizmoTransformChanged);
	// now set the Proxy as the target of the Gizmo
	PlaneTransformGizmo->SetActiveTarget(PlaneTransformProxy, GetToolManager());

	// update the Gizmo with the new Plane/Transform
	PlaneTransformGizmo->ReinitializeGizmoTransform(PlaneTransform);
}


void UMeshPlaneCutTool::OnGizmoTransformChanged(UTransformProxy* Proxy, FTransform Transform)
{
	PlaneTransform = Transform;

	// update the Position and Rotation properties
	Properties->Position = PlaneTransform.GetTranslation();
	Properties->Rotation = PlaneTransform.Rotator();
	// The changes above will trigger the Property Watches on Position and Rotation that 
	// were configured in ::InitializeProperties() on the next Tick. This will result in 
	// unnecessary change events. Doing a SilentUpdate of the PropertySet will prevent that from happening.
	Properties->SilentUpdateWatched();
	// We have changed the property values, but the Editor-level details panel will not pick up
	// these changes automatically, so we notify it here (note: this is expensive! but unavoidable, unfortunately)
	NotifyOfPropertyChangeByTool(Properties);

	// kick off a recompute of the mesh
	InvalidateResult();
}


void UMeshPlaneCutTool::OnShutdown(EToolShutdownType ShutdownType)
{
	// destroy the gizmo we created
	GetToolManager()->GetPairedGizmoManager()->DestroyAllGizmosByOwner(this);

	// save tool settings
	Properties->SaveProperties(this);
}


FText UMeshPlaneCutTool::GetToolMessageString() const
{
	return LOCTEXT("StartToolMessage", "Do things to the current mesh with a Plane.");
}

FText UMeshPlaneCutTool::GetAcceptTransactionName() const
{
	return LOCTEXT("ToolTransactionName", "Plane Op");
}


bool UMeshPlaneCutTool::HasMeshTopologyChanged() const
{
	return true;
}



namespace Local
{

/**
 * FMeshPlaneCutOp actually computes the plane cut. The constructor runs on the game thread
 * (called in MakeNewOperator below) however the CalculateResult function is run from a
 * background compute thread.
 */
class FMeshPlaneCutOp : public FDynamicMeshOperator
{
public:
	struct FOptions
	{
		FTransform LocalToWorld;
		FTransform WorldPlane;
		bool bFillHole;
	};

	FMeshPlaneCutOp(const FDynamicMesh3* Mesh, FOptions Options)
	{
		UseOptions = Options;
		ResultMesh->Copy(*Mesh);		// copy input mesh into output mesh. Do not hold onto input Mesh reference as it is temporary!!
	}

	virtual ~FMeshPlaneCutOp() override {}

	// set ability on protected transform.
	void SetTransform(const FTransformSRT3d& XForm)
	{
		ResultTransform = XForm;
	}

	// base class overrides this.  Results in updated ResultMesh. This function runs in a background thread!!
	virtual void CalculateResult(FProgressCancel* Progress) override
	{
		// transform mesh to world space because that is where plane is (this will correctly handle nonuniform scale)
		MeshTransforms::ApplyTransform(*ResultMesh, (FTransformSRT3d)UseOptions.LocalToWorld);

		FFrame3d Frame(UseOptions.WorldPlane);
		FMeshPlaneCut Cut(ResultMesh.Get(), Frame.Origin, Frame.Z());
		Cut.Cut();

		if (UseOptions.bFillHole)
		{
			Cut.HoleFill(ConstrainedDelaunayTriangulate<double>, false);
		}

		MeshTransforms::ApplyTransformInverse(*ResultMesh, (FTransformSRT3d)UseOptions.LocalToWorld);
	}


protected:
	FOptions UseOptions;
};


}



TUniquePtr<FDynamicMeshOperator> UMeshPlaneCutTool::MakeNewOperator()
{
	// Copy options from the Property Sets. Note that it is not safe to pass the PropertySet directly
	// to the MeshOp because the property set may be modified while the MeshOp computes in the background!
	Local::FMeshPlaneCutOp::FOptions Options;
	Options.LocalToWorld = GetPreviewTransform();
	Options.WorldPlane = PlaneTransform;
	Options.bFillHole = Properties->bFillHole;

	const FDynamicMesh3* Mesh = &GetInitialMesh();
	TUniquePtr<Local::FMeshPlaneCutOp> MeshOp = MakeUnique<Local::FMeshPlaneCutOp>(Mesh, Options);

	FTransform3d XForm3d(GetPreviewTransform());
	MeshOp->SetTransform(XForm3d);

	return MeshOp;
}




#undef LOCTEXT_NAMESPACE