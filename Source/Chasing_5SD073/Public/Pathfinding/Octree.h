#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OctreeNode.h"
#include "OctreeGraph.h"
#include "Octree.generated.h"

UCLASS()
class CHASING_5SD073_API AOctree : public AActor
{
	GENERATED_BODY()
public:
	AOctree();

	virtual void BeginPlay() override;
	
	TArray<OctreeNode*> RootNodes;
	TArray<OctreeNode*> EmptyLeaves;
	OctreeGraph* NavigationGraph;

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
	void ConnectNodes();
	bool DoNodesTouchOnAnyAxis(const OctreeNode* Node1, const OctreeNode* Node2);
};
