#pragma once

#include <future>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OctreeNode.h"
#include "OctreeGraph.h"
#include "Octree.generated.h"

LLM_DECLARE_TAG(OctreeNode);

UCLASS()
class CHASING_5SD073_API AOctree : public AActor
{
	GENERATED_BODY()

public:
	AOctree();

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	TArray<OctreeNode*> RootNodes;
	OctreeGraph* NavigationGraph;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid Settings",
		meta = (AllowPrivateAccess = "true"))
	AActor* ActorToIgnore = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid Settings",
		meta = (AllowPrivateAccess = "true", ClampMin = 1))
	float MinNodeSize = 100;

	// The number of divisions in the grid along the X axis
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid Settings",
		meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 SingleVolumeSize = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid Settings",
		meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 ExpandVolumeXAxis = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid Settings",
		meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 ExpandVolumeYAxis = 1;

	virtual void OnConstruction(const FTransform& Transform) override;


	void MakeOctree(const FVector& Origin);
	void AddObjects(TArray<FOverlapResult> FoundObjects, OctreeNode* RootN) const;
	void GetEmptyNodes(OctreeNode* Node);
	static void AdjustChildNodes(OctreeNode* Node);
	void ConnectNodes() const;
	static bool DoNodesShareFace(const OctreeGraphNode* Node1, const OctreeGraphNode* Node2, float Tolerance);

	UFUNCTION(BlueprintCallable, Category="Octree")
	bool GetAStarPath(const FVector& Start, const FVector& End, FVector& NextLocation) const;

private:
	bool IsSetup = false;
};
