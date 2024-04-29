// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <mutex>

#include "CoreMinimal.h"
#include "Octree.h"
#include "Components/ActorComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "OctreePathfindingComponent.generated.h"


class AOctree;
class FPathfindingRunnableTest;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CHASING_5SD073_API UOctreePathfindingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UOctreePathfindingComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	inline static FLargeMemoryReader* OctreeData = nullptr;
	

	UFUNCTION(BlueprintCallable, Category="Pathfinding")
	void GetAStarPathAsyncToLocation(FVector& TargetLocation, FVector& OutNextDirection);

	UFUNCTION(BlueprintCallable, Category="Pathfinding")
	void GetAStarPathAsyncToTarget(const AActor* TargetActor, FVector& OutNextDirection);

	

	UFUNCTION(BlueprintCallable, Category="Pathfinding")
	void ForceStopPathfinding();

	UFUNCTION(BlueprintCallable, Category="Pathfinding")
	void RestartPathfinding();

	UFUNCTION(BlueprintCallable, Category="Pathfinding")
	void SetOctree(AOctree* NewOctree)
	{
		if (NewOctree == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("New Octree is nullptr. Cannot set Octree."));
			return;
		}
		
		OctreeWeakPtr = NewOctree;
		CollisionChannel = NewOctree->GetCollisionChannel();
		PathfindingRunnable = NewOctree->GetPathfindingRunnable();
	}

private:
	FVector PathSmoothing(const FVector& Start, const AActor* TargetActor, const TArray<FVector>& Path) const;
	void GetAStarPathAsync(const AActor* TargetActor, FVector& TargetLocation, FVector& OutNextDirection);

	UPROPERTY()
	UFloatingPawnMovement* MovementComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pathfinding", meta = (AllowPrivateAccess = "true",  ClampMin = 0))
	float PlayerHeightAimOffset = 0;

	UPROPERTY(EditAnywhere, Category="Pathfinding", meta = (Tooltip = "If the distance to the target is less than this, it will at offset."))
	float StopFloatingAtDistance = 0;

	UPROPERTY(EditAnywhere, Category="Pathfinding")
	float MaxDistanceToTarget = 1000;

	UPROPERTY(EditAnywhere, Category="Pathfinding",
		meta = (Tooltip =
			"Caluclated as such: Speed = (if distance to target > MaxDistanceToTarget) => (DistanceToTarget / MaxDistanceToTarget) * Modifier * Speed"
		))
	float AdaptiveSpeedModifier = 1.0f;

	
	TWeakObjectPtr<AOctree> OctreeWeakPtr;
	TWeakPtr<FPathfindingWorker> PathfindingRunnable;
	ECollisionChannel CollisionChannel = ECC_Visibility;
	
	bool StopPathfinding = false;
	float OriginalSpeed = 0;
	float AgentMeshHalfSize = 0;
	FVector PreviousNextLocation = FVector::ZeroVector;
	TArray<FVector> PathPoints;
};
