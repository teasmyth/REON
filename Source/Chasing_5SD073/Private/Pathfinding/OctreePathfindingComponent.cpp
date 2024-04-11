// Fill out your copyright notice in the Description page of Project Settings.


#include "Pathfinding/OctreePathfindingComponent.h"

#include "GameFramework/FloatingPawnMovement.h"
#include "Pathfinding/OctreeGraph.h"

// Sets default values for this component's properties
UOctreePathfindingComponent::UOctreePathfindingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;


	// ...
}


// Called when the game starts
void UOctreePathfindingComponent::BeginPlay()
{
	Super::BeginPlay();

	AgentMeshHalfSize = GetOwner()->FindComponentByClass<UMeshComponent>()->Bounds.SphereRadius;
	MovementComponent = GetOwner()->FindComponentByClass<UFloatingPawnMovement>();
	OriginalSpeed = MovementComponent->MaxSpeed;
	PreviousNextLocation = GetOwner()->GetActorLocation();

	// ...
}

void UOctreePathfindingComponent::GetAStarPathAsyncToLocation(const AActor* TargetActor, const FVector& TargetLocation, FVector& OutNextDirection)
{
	if (!OctreeWeakPtr.IsValid() || !OctreeWeakPtr->IsOctreeSetup())
	//Worker gets deleted after Setup is set to false, so no need to check for nullptr
	{
		UE_LOG(LogTemp, Warning, TEXT("Octree is not set up. Cannot do pathfinding."));
		OutNextDirection = FVector::ZeroVector;
		return;
	}

	const FVector Start = GetOwner()->GetActorLocation();
	const float Distance = FVector::Dist(Start, TargetLocation);
	constexpr float MinDistanceForPathfinding = 20.0f; //Not meaningful enough to be a public variable, what do I do.

	FHitResult Hit;

	FCollisionShape ColShape = FCollisionShape::MakeSphere(AgentMeshHalfSize);
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(GetOwner());
	TraceParams.AddIgnoredActor(TargetActor);


	if (!GetWorld()->SweepSingleByChannel(Hit, Start, TargetLocation, FQuat::Identity, CollisionChannel, ColShape, TraceParams))
	{
		OutNextDirection = (TargetLocation - Start).GetSafeNormal();
	}


	if (Distance <= MinDistanceForPathfinding)
	{
		UE_LOG(LogTemp, Warning, TEXT("Too close to target."));
		OutNextDirection = FVector::ZeroVector;
		return;
	}

	if (Distance > MaxDistanceToTarget)
	{
		MovementComponent->MaxSpeed = (1 + (Distance / MaxDistanceToTarget) * AdaptiveSpeedModifier) * OriginalSpeed;
	}
	else
	{
		MovementComponent->MaxSpeed = OriginalSpeed;
	}

	//Checking if it's already working. If so, no need to add a new task to the queue.
	if (OctreeWeakPtr->GetPathfindingRunnable()->IsItWorking())
	{
		//Technically we are always using a direction that is calculated in previous frame.
		OutNextDirection = (PreviousNextLocation - Start).GetSafeNormal();
		return;
	}

	//While I did my best to ensure the Octree and its nodes are thread safe, I cannot ensure it with GetWorld as it is handled by the engine.
	//That is why I need to do the path smoothing in the main thread, as it relies on objects that are not thread safe.
	//If we are here, then we are about to start a new  pathfinding thread. This means that we can smooth out the previous one.
	if (OctreeWeakPtr->GetPathfindingRunnable()->GetFoundPath())
	{
		PreviousNextLocation = CalculateNextPathLocation(Start, OctreeWeakPtr->GetPathfindingRunnable()->GetOutQueue());
	}
	/*
	else
	{
		Couldn't find path, -> using previous location. Not finding path usually happens when we are moving through a space that is considered occupied,
		such as, moving diagonally that 'bleeds' into an occupied node. However temporary it is, that technically we are in an occupied node, for that frame
		path cannot be found as we cannot find path from an occupied node. This happens because the path smoothing deems it (physically) empty
		(which is accurate). In that case, we can use previous direction and 'drift' until we are once again inside an OctreeNode.
	}
	*/

	OutNextDirection = (PreviousNextLocation - Start).GetSafeNormal();
	OctreeWeakPtr->GetPathfindingRunnable()->AddToQueue(TPair<FVector, FVector>(Start, TargetLocation), true);
}


bool UOctreePathfindingComponent::GetAStarPathToLocation(const FVector& End, FVector& OutPath)
{
	TArray<FVector> Path;
	const FVector Start = GetOwner()->GetActorLocation();
	const bool PathFound = OctreeGraph::OctreeAStar(false, *OctreeData, Start, End, OctreeWeakPtr->GetRootNode(), Path);

	if (PathFound)
	{
		PreviousNextLocation = CalculateNextPathLocation(Start, Path);
	}

	OutPath = (PreviousNextLocation - Start).GetSafeNormal();

	return PathFound;
}

bool UOctreePathfindingComponent::GetAStarPathToTarget(const AActor* TargetActor, FVector& OutPath)
{
	return GetAStarPathToLocation(TargetActor->GetActorLocation(), OutPath);
}

void UOctreePathfindingComponent::GetAStarPathAsyncToTarget(const AActor* TargetActor, FVector& OutNextLocation)
{
	GetAStarPathAsyncToLocation(TargetActor, TargetActor->GetActorLocation(), OutNextLocation);
}

FVector UOctreePathfindingComponent::CalculateNextPathLocation(const FVector& Start, const TArray<FVector>& Path) const
{
	if (Path.IsEmpty())
	{
		return FVector::ZeroVector;
	}

	if (Path.Num() < 3)
	{
		return Path[0];
	}

	//Path smoothing. If the agent can skip a path point because it wouldn't collide, it should (skip). This ensures a more natural looking movement.
	FHitResult Hit;
	FCollisionShape ColShape = FCollisionShape::MakeSphere(AgentMeshHalfSize);
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(GetOwner());

	for (int i = 1; i < Path.Num(); i++)
	{
		if (!GetWorld()->SweepSingleByChannel(Hit, Start, Path[i], FQuat::Identity, CollisionChannel, ColShape, TraceParams))
		{
			continue;
		}

		return Path[i - 1];
	}

	//If we are here then there was no path smoothing necessary.
	return Path.Last();
}
