#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OctreeNode.h"
#include "Octree.generated.h"

UCLASS()
class CHASING_5SD073_API AOctree : public AActor
{
	GENERATED_BODY()
public:
	AOctree();
	
	OctreeNode RootNode;
	TArray<OctreeNode*> EmptyLeaves;
	//Graph NavigationGraph;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation System Volume|Grid Settings",
		meta = (AllowPrivateAccess = "true", ClampMin = 1))
	float MinNodeSize = 1.0f;

	// The number of divisions in the grid along the X axis
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation System Volume|Grid Settings",
		meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 DivisionsX = 10;

	// The number of divisions in the grid along the Y axis
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation System Volume|Grid Settings",
		meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 DivisionsY = 10;

	// The number of divisions in the grid along the Z axis
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation System Volume|Grid Settings",
		meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 DivisionsZ = 10;

	virtual void OnConstruction(const FTransform& Transform) override;


	void AddObjects(TArray<FOverlapResult> FoundObjects);

};
