#pragma once

#include <future>
#include <mutex>
#include <thread>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OctreeNode.h"
#include "OctreeGraph.h"
#include "Octree.generated.h"


class UProceduralMeshComponent;

UCLASS()
class CHASING_5SD073_API AOctree : public AActor
{
	GENERATED_BODY()

public:
	AOctree();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
#pragma region  Drawing Octree Borders

	// The default (root) scene component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Octree|Drawing", meta = (AllowPrivateAccess = "true"))
	USceneComponent* DefaultSceneComponent = nullptr;

	// The procedural mesh responsible for rendering the grid
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Octree|Drawing", meta = (AllowPrivateAccess = "true"))
	UProceduralMeshComponent* ProceduralMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Octree|Drawing", meta = (AllowPrivateAccess = "true"))
	bool FillBordersOfOctree;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Octree|Drawing", meta = (AllowPrivateAccess = "true"))
	UMaterial* OctreeMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Octree|Drawing", meta = (AllowPrivateAccess = "true"))
	float LineThickness;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Octree|Drawing", meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 1))
	float Opacity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Octree|Drawing", meta = (AllowPrivateAccess = "true"))
	FColor Color;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Octree|Drawing", meta = (AllowPrivateAccess = "true"))
	bool ShowGridAfterCalculation;

	// Gets the size of the grid along the X axis
	float GetOctreeSizeX() const { return SingleVolumeSize + ((ExpandVolumeXAxis - 1) * SingleVolumeSize); }

	// Gets the size of the grid along the Y axis
	float GetOctreeSizeY() const { return SingleVolumeSize + ((ExpandVolumeYAxis - 1) * SingleVolumeSize); }

	// Gets the size of the grid along the Z axis
	float GetOctreeSizeZ() const { return SingleVolumeSize + ((ExpandVolumeZAxis - 1) * SingleVolumeSize); }

	void CreateCubeMesh(const FVector& Corner1, const FVector& Corner2, const FVector& Corner3, const FVector& Corner4, const FVector& Corner5,
	                    const FVector& Corner6, const FVector& Corner7, const FVector& Corner8);

	static void CreateLine(const FVector& Start, const FVector& End, const FVector& Normal, TArray<FVector>& Vertices, TArray<int32>& Triangles,
	                       const float& LineThickness);


	void ShowGrid();

#pragma endregion


	TArray<OctreeNode*> RootNodes;
	OctreeGraph* NavigationGraph;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Octree", meta = (AllowPrivateAccess = "true"))
	TArray<AActor*> ActorToIgnore;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Octree", meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<ECollisionChannel> CollisionChannel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Octree", meta = (AllowPrivateAccess = "true"))
	bool AutoEncapsulateObjects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Octree", meta = (AllowPrivateAccess = "true"))
	bool UsePhysicsOverlap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Octree",
		meta = (AllowPrivateAccess = "true", ClampMin = 1))
	float MinNodeSize = 100;

	// The number of divisions in the grid along the X axis
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Octree", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 SingleVolumeSize = 1600;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Octree", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 ExpandVolumeXAxis = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Octree", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 ExpandVolumeYAxis = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Octree", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 ExpandVolumeZAxis = 1;


	void SetUpOctreesAsync();
	void MakeOctree(const FVector& Origin);
	void AddObjects(TArray<FOverlapResult> FoundObjects, OctreeNode* RootN) const;
	void GetEmptyNodes(OctreeNode* Node) const;
	static void AdjustChildNodes(OctreeNode* Node);
	void DrawOctreeBorders();

	UFUNCTION(BlueprintCallable, Category="Octree")
	bool GetAStarPath(const AActor* Agent, const FVector& End, FVector& OutNextLocation);

	UFUNCTION(BlueprintCallable, Category="Octree")
	bool GetAStarPathToTarget(const AActor* Agent, const AActor* End, FVector& NextLocation);


	UFUNCTION(BlueprintCallable, Category="Octree")
	void GetAStarPathAsyncToLocation(const AActor* Agent, const FVector& Target, FVector& OutNextDirection);

	UFUNCTION(BlueprintCallable, Category="Octree")
	void GetAStarPathAsyncToTarget(const AActor* Agent, const AActor* Target, FVector& OutNextLocation);

	std::atomic<bool> IsSetup = false;
	float AgentMeshHalfSize = 0;

	FVector PreviousNextLocation = FVector::ZeroVector;

	std::unique_ptr<std::thread> PathfindingThread;
	std::mutex PathfindingMutex;
	bool IsPathfindingInProgress = false;
};
