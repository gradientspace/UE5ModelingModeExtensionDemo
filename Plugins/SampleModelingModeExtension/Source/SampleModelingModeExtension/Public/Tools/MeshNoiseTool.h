// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "CoreMinimal.h"
#include "BaseTools/BaseMeshProcessingTool.h"
#include "MeshNoiseTool.generated.h"


UENUM()
enum class EMeshNoiseToolNoiseType : uint8
{
	Random,
	Perlin
};


UCLASS()
class SAMPLEMODELINGMODEEXTENSION_API UMeshNoiseProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:

	/** Number of edge subdivisions */
	UPROPERTY(EditAnywhere, Category = Subdivisions, meta = (UIMin = "0", UIMax = "5", ClampMin = "0", ClampMax = "10"))
	int Subdivisions = 0;

	UPROPERTY(EditAnywhere, Category = Noise)
	EMeshNoiseToolNoiseType NoiseType = EMeshNoiseToolNoiseType::Perlin;

	/** Scale of the noise */
	UPROPERTY(EditAnywhere, Category = Noise, meta = (UIMin = "-10.0", UIMax = "10.0", ClampMin = "-1000.0", ClampMax = "100.0"))
	float Scale = 5.0f;

	/** Frequency of the noise */
	UPROPERTY(EditAnywhere, Category = Noise, meta = (UIMin = "0", UIMax = "5.0", EditCondition = "NoiseType == EMeshNoiseToolNoiseType::Perlin"))
	float Frequency = 1.0f;

	/** Random Seed */
	UPROPERTY(EditAnywhere, Category = Noise, meta = (UIMin = "0", ClampMin = "0", EditCondition = "NoiseType == EMeshNoiseToolNoiseType::Random"))
	int Seed = 10;

};



/**
 * UMeshNoiseTool applies PN Tessellation and Perlin or Random noise to an input Mesh
 */
UCLASS()
class SAMPLEMODELINGMODEEXTENSION_API UMeshNoiseTool : public UBaseMeshProcessingTool
{
	GENERATED_BODY()

public:
	UMeshNoiseTool();

protected:
	// UBaseMeshProcessingTool API implementation

	virtual void InitializeProperties() override;
	virtual void OnShutdown(EToolShutdownType ShutdownType) override;

	virtual TUniquePtr<UE::Geometry::FDynamicMeshOperator> MakeNewOperator() override;

	virtual bool RequiresInitialVtxNormals() const { return true; }
	virtual bool HasMeshTopologyChanged() const override;

	virtual FText GetToolMessageString() const override;
	virtual FText GetAcceptTransactionName() const override;

	// disable scaling to unit dimensions, this is a feature of UBaseMeshProcessingTool that is enabled by default
	virtual bool RequiresScaleNormalization() const { return false; }

protected:
	//  settings for this Tool that will be exposed in Modeling Mode details panel
	UPROPERTY()
	TObjectPtr<UMeshNoiseProperties> NoiseProperties = nullptr;
};




UCLASS()
class SAMPLEMODELINGMODEEXTENSION_API UMeshNoiseToolBuilder : public UBaseMeshProcessingToolBuilder
{
	GENERATED_BODY()
public:
	virtual UBaseMeshProcessingTool* MakeNewToolInstance(UObject* Outer) const 
	{
		return NewObject<UMeshNoiseTool>(Outer);
	}
};
