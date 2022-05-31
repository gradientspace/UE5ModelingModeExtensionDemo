// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "Tools/MeshNoiseTool.h"
#include "InteractiveToolManager.h"
#include "ToolBuilderUtil.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/MeshNormals.h"
#include "Operations/PNTriangles.h"
#include "ModelingOperators.h"

using namespace UE::Geometry;

#define LOCTEXT_NAMESPACE "UMeshNoiseTool"




UMeshNoiseTool::UMeshNoiseTool()
{
	SetToolDisplayName(LOCTEXT("ToolName", "Noise"));
}



void UMeshNoiseTool::InitializeProperties()
{
	NoiseProperties = NewObject<UMeshNoiseProperties>(this);
	AddToolPropertySource(NoiseProperties);
	NoiseProperties->RestoreProperties(this);
	NoiseProperties->WatchProperty(NoiseProperties->Subdivisions, [&](int) { InvalidateResult();  });
	NoiseProperties->WatchProperty(NoiseProperties->NoiseType, [&](EMeshNoiseToolNoiseType) { InvalidateResult();  });
	NoiseProperties->WatchProperty(NoiseProperties->Scale, [&](float) { InvalidateResult();  });
	NoiseProperties->WatchProperty(NoiseProperties->Frequency, [&](float) { InvalidateResult();  });
	NoiseProperties->WatchProperty(NoiseProperties->Seed, [&](int NewSeed) { InvalidateResult();  });
}



void UMeshNoiseTool::OnShutdown(EToolShutdownType ShutdownType)
{
	NoiseProperties->SaveProperties(this);
}


FText UMeshNoiseTool::GetToolMessageString() const
{
	return LOCTEXT("StartMeshNoiseMessage", "Add noise to the mesh vertex positions.");
}

FText UMeshNoiseTool::GetAcceptTransactionName() const
{
	return LOCTEXT("MeshNoiseToolTransactionName", "Mesh Noise");
}


bool UMeshNoiseTool::HasMeshTopologyChanged() const
{
	return false;
}



namespace Local
{

/**
 * FMeshPlaneCutOp actually computes the mesh deformation. The constructor runs on the game thread
 * (called in MakeNewOperator below) however the CalculateResult function is run from a
 * background compute thread.
 */
class FMeshNoiseOp : public FDynamicMeshOperator
{
public:
	struct FOptions
	{
		int32 Subdivisions = 0;

		EMeshNoiseToolNoiseType NoiseType;
		double Magnitude = 1.0;
		int32 RandomSeed = 0;
		double Frequency = 1.0;
	};

	TSharedPtr<FMeshNormals> BaseMeshNormals;

	FMeshNoiseOp(const FDynamicMesh3* Mesh, FOptions Options)
	{
		UseOptions = Options;
		ResultMesh->Copy(*Mesh);		// copy input mesh into output mesh. Do not hold onto input Mesh reference as it is temporary!!
	}

	virtual ~FMeshNoiseOp() override {}

	// set ability on protected transform.
	void SetTransform(const FTransformSRT3d& XForm)
	{
		ResultTransform = XForm;
	}

	// Called on background thread to compute the mesh op result. 
	// The input mesh is stored and returned via .ResultMesh member.
	// .ResultInfo member is used to indicate success/failure
	virtual void CalculateResult(FProgressCancel* Progress) override
	{
		ResultInfo = FGeometryResult();

		FMeshNormals SubdividedMeshNormals;

		// If subdivisions were requested, compute it
		// (Excercise for the reader: This mesh could be cached at the Tool level...)
		if (UseOptions.Subdivisions > 0)
		{
			FPNTriangles PNTriangles(ResultMesh.Get());
			PNTriangles.TessellationLevel = UseOptions.Subdivisions;
			PNTriangles.Progress = Progress;
			if (PNTriangles.Validate() == EOperationValidationResult::Ok)
			{
				PNTriangles.Compute();
			}

			// once we have subdivided, we will need to recompute vertex normals on the base mesh...
			if (Progress->Cancelled() == false)
			{
				SubdividedMeshNormals = FMeshNormals(ResultMesh.Get());
				SubdividedMeshNormals.ComputeVertexNormals();
			}
		}

		// abort if we were cancelled
		if (ResultInfo.CheckAndSetCancelled(Progress))
		{
			return;
		}

		// create stream for randomization
		FRandomStream Stream(UseOptions.RandomSeed);

		// Loop over vertices and update positions in-place
		for (int32 vid : ResultMesh->VertexIndicesItr())
		{
			FVector3d Position = ResultMesh->GetVertex(vid);
			FVector3d Normal = (UseOptions.Subdivisions > 0) ? SubdividedMeshNormals[vid] : (*BaseMeshNormals)[vid];

			FVector3d NewPosition = Position;
			if (UseOptions.NoiseType == EMeshNoiseToolNoiseType::Random)
			{
				double RandomAlpha = Stream.GetFraction();
				NewPosition = Position + (UseOptions.Magnitude * RandomAlpha * Normal);
			}
			else
			{
				// Frequency is manipulated here to provide a nicer range for the slider. This is scale-dependent, though!
				double NoiseValue = FMath::PerlinNoise3D( FMathd::Pow(UseOptions.Frequency * 0.1, 2.0) * Position);
				NewPosition = Position + (UseOptions.Magnitude * NoiseValue * Normal);
			}
			
			ResultMesh->SetVertex(vid, NewPosition);

			// don't check for cancel every iteration because it is somewhat expensive
			if ( vid % 1000 && ResultInfo.CheckAndSetCancelled(Progress))
			{
				return;
			}
		}

		// recalculate normals
		if (Progress->Cancelled() == false)
		{
			if (ResultMesh->HasAttributes())
			{
				FMeshNormals::QuickRecomputeOverlayNormals(*ResultMesh);
			}
			else
			{
				FMeshNormals::QuickComputeVertexNormals(*ResultMesh);
			}
		}

		ResultInfo.SetSuccess(true, Progress);
	}


protected:
	FOptions UseOptions;
};


}



TUniquePtr<FDynamicMeshOperator> UMeshNoiseTool::MakeNewOperator()
{
	// Copy options from the Property Sets. Note that it is not safe to pass the PropertySet directly
	// to the MeshOp because the property set may be modified while the MeshOp computes in the background!
	Local::FMeshNoiseOp::FOptions Options;
	Options.Magnitude = NoiseProperties->Scale;
	Options.NoiseType = NoiseProperties->NoiseType;
	Options.RandomSeed = NoiseProperties->Seed;
	Options.Frequency = NoiseProperties->Frequency;
	Options.Subdivisions = NoiseProperties->Subdivisions;

	TUniquePtr<Local::FMeshNoiseOp> MeshOp = MakeUnique<Local::FMeshNoiseOp>(&GetInitialMesh(), Options);
	MeshOp->SetTransform( (FTransform3d)GetPreviewTransform() );
	MeshOp->BaseMeshNormals = GetInitialVtxNormals();

	return MeshOp;
}




#undef LOCTEXT_NAMESPACE