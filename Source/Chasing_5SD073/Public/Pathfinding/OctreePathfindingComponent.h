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
	bool GetAStarPathToLocation(const FVector& End, FVector& OutPath) ;

	UFUNCTION(BlueprintCallable, Category="Pathfinding")
	bool GetAStarPathToTarget(const AActor* TargetActor, FVector& OutPath);

	UFUNCTION(BlueprintCallable, Category="Pathfinding")
	void GetAStarPathAsyncToLocation(const AActor* TargetActor, const FVector& TargetLocation, FVector& OutNextDirection);

	UFUNCTION(BlueprintCallable, Category="Pathfinding")
	void GetAStarPathAsyncToTarget(const AActor* TargetActor, FVector& OutNextLocation);

	UFUNCTION(BlueprintCallable, Category="Pathfinding")
	void ForceStopPathfinding();

	UFUNCTION(BlueprintCallable, Category="Pathfinding")
	void RestartPathfinding();

	UFUNCTION(BlueprintCallable, Category="Pathfinding")
	void SetOctree(AOctree* NewOctree)
	{
		OctreeWeakPtr = NewOctree;
		CollisionChannel = NewOctree->GetCollisionChannel();
		PathfindingRunnable = NewOctree->GetPathfindingRunnable();
	}

private:
	FVector CalculateNextPathLocation(const FVector& Start, const AActor* TargetActor, const TArray<FVector>& Path) const;

	UPROPERTY()
	UFloatingPawnMovement* MovementComponent = nullptr;

	UPROPERTY(EditAnywhere, Category="Pathfinding", meta = (Tooltip = "If it can, it will try to float above the ground by this amount."))
	float FloatAboveGround = 0;

	UPROPERTY(EditAnywhere, Category="Pathfinding", meta = (Tooltip = "If the distance to the target is less than this, it will stop floating."))
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
	ECollisionChannel CollisionChannel = ECollisionChannel::ECC_Visibility;
	
	bool StopPathfinding = false;
	float OriginalSpeed = 0;
	float AgentMeshHalfSize = 0;
	FVector PreviousNextLocation = FVector::ZeroVector;
	TArray<FVector> PathPoints;
};
