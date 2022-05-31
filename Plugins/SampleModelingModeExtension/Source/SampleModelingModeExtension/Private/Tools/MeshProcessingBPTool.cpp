// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "Tools/MeshProcessingBPTool.h"
#include "InteractiveToolManager.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/MeshNormals.h"
#include "Operations/PNTriangles.h"
#include "ModelingOperators.h"
#include "Util/ProgressCancel.h"
#include "UObject/StrongObjectPtr.h"

using namespace UE::Geometry;

#define LOCTEXT_NAMESPACE "UMeshProcessingBPTool"



/**
 * FBackgroundMeshProcessingExecutor is a helper class that the UMeshProcessingBPTool and 
 * FBPMeshProcessingOp's can use to force the UMeshProcessingBPToolOperation blueprints subclasses
 * to execute on the Game Thread. Basically if the UMeshProcessingBPToolOperation::GetEnableBackgroundExecution()
 * function returns true (default false), then instead of directly calling UMeshProcessingBPToolOperation::OnRecomputeMesh,
 * the Op will call QueueForMainThread() below, and then busy-wait. The Tool calls ExecuteOneOperationOnGameThread()
 * once per tick, allowing a BP execution on the game thread, which then cancels the busy-wait
 * in the background-thread FBPMeshProcessingOp, allowing it to complete (if not cancelled). 
 * 
 * FBackgroundMeshProcessingExecutor is also used to keep the UMeshProcessingBPToolOperation and UDynamicMesh
 * for a pending FBPMeshProcessingOp execution alive while the background thread needs them.
 * Something needs to do this, otherwise they will be randomly garbage-collected, as UMeshProcessingBPTool
 * does not directly reference these temp objects (and cannot easily, as multiple may be active).
 * So, we make this class a FGCObject, and have it own references to the active Operations/DynamicMeshes.
 * This all works relatively well, except that WaitForAllPendingToFinish() may block Tool shutdown
 * while it waits for pending operations to be completed.
 */
class FBackgroundMeshProcessingExecutor : public FGCObject
{
public:
	// FBPMeshProcessingOp implements this interface so that we can force it's BP to be executed
	class IExecuteTarget
	{
	public:
		virtual void ExecuteBlueprint(FProgressCancel* Progress) = 0;
	};

	// just a handy tuple struct
	struct FPendingOperation
	{
		IExecuteTarget* Target;
		FProgressCancel* Progress;
	};

	// list of background executions that are busy-waiting for their BPs to be executed
	TArray<FPendingOperation> PendingOperations;
	FCriticalSection PendingLock;

	// FBPMeshProcessingOp calls this from background thread during CalculateResult() to
	// request a game-thread BP execution
	void QueueForMainThread(FPendingOperation Operation)
	{
		PendingLock.Lock();
		PendingOperations.Add(Operation);
		PendingLock.Unlock();
	}

	// UMeshProcessingBPTool calls this once per Tick to do a game-thread BP execution
	void ExecuteOneOperationOnGameThread()
	{
		PendingLock.Lock();
		if (PendingOperations.Num() > 0)
		{
			FPendingOperation Operation = PendingOperations.Pop(false);
			Operation.Target->ExecuteBlueprint(Operation.Progress);
		}
		PendingLock.Unlock();
	}

	// call this to clear out any pending computes
	void ClearOnToolShutdown()
	{
		PendingLock.Lock();
		for (FPendingOperation Operation : PendingOperations)
		{
			Operation.Target->ExecuteBlueprint(Operation.Progress);
		}
		PendingLock.Unlock();
	}



	TArray<UDynamicMesh*> TempMeshes;
	FCriticalSection TempMeshesLock;

	TArray<UMeshProcessingBPToolOperation*> TempOperations;
	FCriticalSection TempOperationsLock;

	virtual FString GetReferencerName() const override { return TEXT("FBackgroundMeshProcessingExecutor"); }

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		TempMeshesLock.Lock();
		for (UDynamicMesh* Mesh : TempMeshes)
		{
			Collector.AddReferencedObject(Mesh);
		}
		TempMeshesLock.Unlock();

		TempOperationsLock.Lock();
		for (UMeshProcessingBPToolOperation* Operation: TempOperations)
		{
			Collector.AddReferencedObject(Operation);
		}
		TempOperationsLock.Unlock();
	}

	virtual void AddTempMesh(UDynamicMesh* Mesh)
	{
		TempMeshesLock.Lock();
		check(TempMeshes.Contains(Mesh) == false);
		TempMeshes.Add(Mesh);
		TempMeshesLock.Unlock();
	}

	virtual void ReleaseTempMesh(UDynamicMesh* Mesh)
	{
		TempMeshesLock.Lock();
		check(TempMeshes.Contains(Mesh));
		TempMeshes.Remove(Mesh);
		TempMeshesLock.Unlock();
	}

	virtual void AddTempOperation(UMeshProcessingBPToolOperation* Operation)
	{
		TempOperationsLock.Lock();
		check(TempOperations.Contains(Operation) == false);
		TempOperations.Add(Operation);
		TempOperationsLock.Unlock();
	}

	virtual void ReleaseTempOperation(UMeshProcessingBPToolOperation* Operation)
	{
		TempOperationsLock.Lock();
		check(TempOperations.Contains(Operation));
		TempOperations.Remove(Operation);
		TempOperationsLock.Unlock();
	}

	virtual void WaitForAllPendingToFinish()
	{
		bool bDone = false;
		while (!bDone)
		{
			bDone = true;
			TempMeshesLock.Lock();
			bDone = bDone && (TempMeshes.Num() == 0);
			TempMeshesLock.Unlock();
			TempOperationsLock.Lock();
			bDone = bDone && (TempOperations.Num() == 0);
			TempOperationsLock.Unlock();

			if (!bDone)
			{
				FPlatformProcess::Sleep(0.25f);
			}
		}
	}
};



// By default, a UMeshProcessingBPToolOperation BP-subclass can only be executed on the game thread (safest)
bool UMeshProcessingBPToolOperation::GetEnableBackgroundExecution_Implementation()
{
	return false;
}



UMeshProcessingBPTool::UMeshProcessingBPTool()
{
	SetToolDisplayName(LOCTEXT("ToolName", "Noise"));
}

void UMeshProcessingBPTool::InitializeProperties()
{
	Properties = NewObject<UMeshProcessingBPToolProperties>(this);
	AddToolPropertySource(Properties);
	Properties->RestoreProperties(this);

	Executor = MakeShared<FBackgroundMeshProcessingExecutor>();
}

void UMeshProcessingBPTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{
	InvalidateResult();
}

void UMeshProcessingBPTool::OnTick(float DeltaTime)
{
	UBaseMeshProcessingTool::OnTick(DeltaTime);

	Executor->ExecuteOneOperationOnGameThread();
}

void UMeshProcessingBPTool::OnShutdown(EToolShutdownType ShutdownType)
{
	Executor->ClearOnToolShutdown();
	Executor->WaitForAllPendingToFinish();

	Properties->SaveProperties(this);
}


FText UMeshProcessingBPTool::GetToolMessageString() const
{
	return LOCTEXT("StartMeshNoiseMessage", "Add noise to the mesh vertex positions.");
}

FText UMeshProcessingBPTool::GetAcceptTransactionName() const
{
	return LOCTEXT("MeshProcessingBPToolTransactionName", "Mesh Noise");
}


bool UMeshProcessingBPTool::HasMeshTopologyChanged() const
{
	return true;
}



namespace Local
{

class FBPMeshProcessingOp : public FDynamicMeshOperator, public FBackgroundMeshProcessingExecutor::IExecuteTarget
{
public:
	struct FOptions
	{
		// TStrongObjectPtr is ugly here but Operation and TempMesh need to be kept alive for lifetime of the Op
		UMeshProcessingBPToolOperation* Operation;
		FMeshProcessingBPToolParameters Settings;

		UDynamicMesh* TempMesh;

		TSharedPtr<FBackgroundMeshProcessingExecutor> Executor;
	};

	// If executing BP on game thread from background compute thread, the op will busy-wait for
	// this boolean to become true
	std::atomic<bool> bBlueprintExecuted;

	FBPMeshProcessingOp(const FDynamicMesh3* Mesh, FOptions Options)
	{
		UseOptions = Options;
		ResultMesh->Copy(*Mesh);		// copy input mesh into output mesh. Do not hold onto input Mesh reference as it is temporary!!

		bBlueprintExecuted = false;
	}

	virtual ~FBPMeshProcessingOp() override {}

	// set ability on protected transform.
	void SetTransform(const FTransformSRT3d& XForm)
	{
		ResultTransform = XForm;
	}

	// this may be called on Game Thread or in background compute thread, depending on Operation requirements
	virtual void ExecuteBlueprint(FProgressCancel* Progress)
	{
		// if the Operation cannot be executed in a background thread, make sure we are in the game thread...
		bool bSkipOperation = false;
		if (UseOptions.Operation->GetEnableBackgroundExecution() == false)
		{
			if ( ! ensure(IsInGameThread()) )
			{
				bSkipOperation = true;
			}
		}
		else
		{
			ensure(IsInGameThread() == false);		// just a sanity check to make sure we are actually in a background thread...
		}

		if (bSkipOperation == false && Progress->Cancelled() == false)
		{
			UseOptions.Operation->OnRecomputeMesh(UseOptions.TempMesh, UseOptions.Settings);
		}

		bBlueprintExecuted = true;		// indicate that we have completed work, to end busy wait in CalculateResult()
	}

	// Called on background thread to compute the mesh op result. 
	// The input mesh is stored and returned via .ResultMesh member.
	// .ResultInfo member is used to indicate success/failure
	virtual void CalculateResult(FProgressCancel* Progress) override
	{
		ResultInfo = FGeometryResult();

		// abort if we don't have valid inputs
		if (UseOptions.Operation == nullptr)
		{
			ResultInfo.SetSuccess(false , Progress);
			UseOptions.Executor->ReleaseTempMesh(UseOptions.TempMesh);
			return;
		}

		// can this ever happen?
		check(UseOptions.TempMesh != nullptr);

		UseOptions.TempMesh->SetMesh(MoveTemp(*ResultMesh));

		if (UseOptions.Operation->GetEnableBackgroundExecution())
		{
			ExecuteBlueprint(Progress);
		}
		else
		{
			// if we cannot execute on background, push to game thread and then wait until it has been executed
			UseOptions.Executor->QueueForMainThread( FBackgroundMeshProcessingExecutor::FPendingOperation{ this, Progress } );
			while (bBlueprintExecuted == false)
			{
				FPlatformProcess::Sleep(0.01f);
			}
		}

		if (Progress->Cancelled() == false)
		{
			TUniquePtr<UE::Geometry::FDynamicMesh3> EditedMesh = UseOptions.TempMesh->ExtractMesh();
			*ResultMesh = MoveTemp(*EditedMesh);
		}

		UseOptions.Executor->ReleaseTempOperation(UseOptions.Operation);
		UseOptions.Executor->ReleaseTempMesh(UseOptions.TempMesh);

		ResultInfo.SetSuccess(true, Progress);
	}


protected:
	FOptions UseOptions;
};


}



TUniquePtr<FDynamicMeshOperator> UMeshProcessingBPTool::MakeNewOperator()
{
	// spawn a new instance of the Operation BP type, if it is set
	UMeshProcessingBPToolOperation* OperationInstance = nullptr;
	if (Properties->Operation != nullptr)
	{
		UClass* ClassType = Properties->Operation;
		OperationInstance = NewObject<UMeshProcessingBPToolOperation>((UObject*)GetTransientPackage(), ClassType);
		this->Executor->AddTempOperation(OperationInstance);
	}

	Local::FBPMeshProcessingOp::FOptions Options;
	Options.Operation = OperationInstance;
	Options.TempMesh = NewObject<UDynamicMesh>(this);		// TODO: this could come from a pool, to avoid UObject garbage spew
	this->Executor->AddTempMesh(Options.TempMesh);
	Options.Settings = Properties->Parameters;
	Options.Executor = this->Executor;

	TUniquePtr<Local::FBPMeshProcessingOp> MeshOp = MakeUnique<Local::FBPMeshProcessingOp>(&GetInitialMesh(), Options);
	MeshOp->SetTransform( (FTransform3d)GetPreviewTransform() );

	return MeshOp;
}




#undef LOCTEXT_NAMESPACE