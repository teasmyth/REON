// Fill out your copyright notice in the Description page of Project Settings.


#include "NavSystemComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NavSystemNode.h"
#include <set>
#include <unordered_map>
#include <unordered_set>
#include "queue"

// Sets default values for this component's properties
UNavSystemComponent::UNavSystemComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UNavSystemComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}

bool UNavSystemComponent::DecideNavigation(FVector& OutDirectionToMove)
{
	if (AgentVolume && TargetVolume)
	{
		return false;
	}

	if (Target == nullptr)
	{
		return true; //For safety reasons, in case it doesnt have reference (yet), doesnt crash the game.
	}

	FCollisionShape HitBox = FCollisionShape::MakeBox(FVector(32));
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(GetOwner());
	if (TargetVolume != nullptr) //Move towards the enter point of target's volume.
	{
		FHitResult HitResult;
		bool bHit = GetWorld()->SweepSingleByChannel(HitResult, GetOwner()->GetActorLocation(), Target->GetActorLocation(), FQuat::Identity,
		                                             ECC_GameTraceChannel1, HitBox, TraceParams);
		if (bHit && HitResult.GetActor() == Target)
		{
			OutDirectionToMove = (Target->GetActorLocation() - GetOwner()->GetActorLocation()).GetSafeNormal();
		}
		else
		{
			OutDirectionToMove = (TargetVolume->GetTargetActorEnterLocation() - GetOwner()->GetActorLocation()).GetSafeNormal();
		}
	}
	else //In 'free space,' just move towards the target.
	{
		OutDirectionToMove = (Target->GetActorLocation() - GetOwner()->GetActorLocation()).GetSafeNormal();
	}
	return true;
}

// Called every frame
void UNavSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

bool UNavSystemComponent::FindPath(const FVector& Start, AActor* target,
                                   const TArray<TEnumAsByte<EObjectTypeQuery>>& Object_Types,
                                   const float& MeshBounds, UClass* Actor_Class_Filter,
                                   FVector& OutDirectionToMove, const bool& bUseAStar, const ECollisionChannel& CollisionChannel)

{
	const ANavSystemVolume* NavVolume = AgentVolume; //Too lazy to rewrite all.

	if (!NavVolume->GetAreNodesLoaded() || Target == nullptr)
	{
		return false;
	}

	bool drawdebugenabled = false;

	// Create open and closed sets
	std::priority_queue<NavSystemNode*, std::vector<NavSystemNode*>, FNodeCompare> OpenSet;
	std::set<NavSystemNode*> OpenSetCheck; //For checking if something is in open set.
	std::set<NavSystemNode*> ClosedSet;
	std::unordered_map<NavSystemNode*, NavSystemNode*> ParentMap; //parent system, to tell where can you come from to a particular node

#pragma region Helper methods

	// Reconstruct the path from the cameFrom map
	auto ReconstructPath = [&](const std::unordered_map<NavSystemNode*, NavSystemNode*>& cameFrom, NavSystemNode* current, FVector& OutDirection)
	{
		TArray<FVector> Path;

		while (current)
		{
			Path.Insert(ConvertCoordinatesToLocation(*NavVolume, current->Coordinates), 0);
			//DrawDebugBox(GetWorld(), ConvertCoordinatesToLocation(*NavVolume, current->Coordinates), FVector(meshBounds / 2), FColor::Blue, false, 1,
			//           0, 5.0f);
			current = cameFrom.count(current) ? cameFrom.at(current) : nullptr;
		}

		/*
		if (Path.Num() > 2) //Smoothing path if its not right next to it.
		{
			TArray<FVector> SmoothenedPath;
			ETraceTypeQuery TraceChannel = UEngineTypes::ConvertToTraceType(ECC_Visibility); // Specify the trace channel using ECC_Visibility
			FVector currentPoint = *Path.begin();
			FCollisionShape HitBox = FCollisionShape::MakeBox(FVector(meshBounds * 1.1f));
			SmoothenedPath.Add(currentPoint);
			FCollisionQueryParams TraceParams;
			TraceParams.AddIgnoredActor(GetOwner());

			//out path num = 9  -> loop max is 8,
			for (int i = 0; i < Path.Num() - 1; i++)
			{
				FHitResult hitResult;

				//if (UKismetSystemLibrary::SphereTraceSingle(GetWorld(), currentPoint, Path[i + 1], meshBounds, TraceChannel, false,
				//                                            TArray<AActor*>(), EDrawDebugTrace::Type::None, hitResult, true))
				if (GetWorld()->SweepSingleByChannel(hitResult, currentPoint, Path[i + 1], FQuat::Identity, CollisionChannel, HitBox, TraceParams))
				{
					//DrawDebugLine(GetWorld(), currentPoint, Path[i + 1], FColor::Red, false, 1, 0, 5);
					currentPoint = Path[i + 1];
					SmoothenedPath.Add(Path[i + 1]);
				//	DrawDebugBox(GetWorld(), currentPoint, FVector(meshBounds), FColor::Red, false, 1, 0, 5.0f);
				}
			}

			UE_LOG(LogTemp, Display, TEXT("Normal path point count: %i"), Path.Num());
			SmoothenedPath.Add(Path.Last());
			UE_LOG(LogTemp, Display, TEXT("Smooth path point count: %i"), SmoothenedPath.Num());

			Path = SmoothenedPath;
		}
		*/
		if (Path.Num() > 1)
		{
			OutDirection = Path[1] - GetOwner()->GetActorLocation();
		}
		else
		{
			OutDirection = FVector::ZeroVector;

			//If the target volume is zero and the path dir is also zero, we reached the end, unload the agent volume.
			if (TargetVolume == nullptr) AgentVolume = nullptr;
		}
		OutDirection = OutDirection.GetSafeNormal();
	};

	auto CalculateTentativeGScore = [&](const NavSystemNode& current, const NavSystemNode& neighbor)
	{
		int deltaX = FMath::Abs(neighbor.Coordinates.X - current.Coordinates.X);
		int deltaY = FMath::Abs(neighbor.Coordinates.Y - current.Coordinates.Y);
		int deltaZ = FMath::Abs(neighbor.Coordinates.Z - current.Coordinates.Z);

		// Check for diagonal movement
		if ((deltaX != 0 && deltaY != 0 && deltaZ == 0) || // Diagonal along X and Y axes
			(deltaX != 0 && deltaY == 0 && deltaZ != 0) || // Diagonal along X and Z axes
			(deltaX == 0 && deltaY != 0 && deltaZ != 0)) // Diagonal along Y and Z axes
		{
			// Diagonal movement: Add a higher cost (e.g., 14)
			return current.GScore + 140; // + AdditionalCosts(current, neighbor); // Consider terrain costs, etc.
		}

		// Horizontal or vertical movement: Add a lower cost (e.g., 10)
		return current.GScore + 100; // + AdditionalCosts(current, neighbor); // Consider terrain costs, etc.
	};


	auto Cleanup = [&]()
	{
		for (NavSystemNode* item : OpenSetCheck)
		{
			item->FScore = item->GScore = item->HScore = FLT_MAX;
		}
		for (NavSystemNode* item : ClosedSet)
		{
			item->FScore = item->GScore = item->HScore = FLT_MAX;
		}
	};

	auto IsBlocked = [&](const NavSystemNode* nodeToCheck)
	{
		TArray<AActor*> outActors;
		return UKismetSystemLibrary::BoxOverlapActors(GetWorld(), ConvertCoordinatesToLocation(*NavVolume, nodeToCheck->Coordinates),
		                                              FVector(MeshBounds) * 1.1f, Object_Types,
		                                              Actor_Class_Filter, TArray<AActor*>(), outActors);
	};

#pragma  endregion

	// Initialize start node
	NavSystemNode* StartNode = GetNode(*NavVolume, ConvertLocationToCoordinates(*NavVolume, Start));

	/*
	FIntVector origin = ConvertLocationToCoordinates(*NavVolume, destination);
	FIntVector up = origin + FIntVector(0, 0, 1);
	*/

	const NavSystemNode* EndNode = GetNode(*NavVolume, ConvertLocationToCoordinates(*NavVolume, Target->GetActorLocation()));

	/*
		//bug fix: when player is sharing a grid that's blocked, target the neighbor grid that's free.
		if (IsBlocked(EndNode))
		{
			for (NavSystemNode* neighbor : EndNode->Neighbors)
			{
				if (!IsBlocked(neighbor))
				{
					EndNode = neighbor;
					break;
				}
			}
		}
	*/

	StartNode->GScore = 0;
	StartNode->HScore = 0;
	StartNode->FScore = 0;

	OpenSet.push(StartNode);
	OpenSetCheck.insert(StartNode);
	int LoopIndex = 0;
	
	while (!OpenSet.empty() && LoopIndex < 100) //Gotta keep it a bit big in case the distance is too big. Low also for not to be laggy.
	{
		LoopIndex++;
		NavSystemNode* CurrentNode = OpenSet.top();
		OpenSet.pop();
		ClosedSet.emplace(CurrentNode);

		if (CurrentNode->Coordinates == EndNode->Coordinates)
		{
			ReconstructPath(ParentMap, CurrentNode, OutDirectionToMove);
			Cleanup();
			return true;
		}
		
		if (CurrentNode->Neighbors.size() == 0) AddNeighbors(*NavVolume, CurrentNode);

		NavSystemNode* JumpPoint = nullptr;
		for (auto Neighbor : CurrentNode->Neighbors)
		{
			//If true, it means it was a successful jump.
			if (Jump(*NavVolume, *CurrentNode, *Neighbor, target, MeshBounds, CollisionChannel, &JumpPoint))
			{
				//Perhaps there is a better way, but for now, just calculating based on F yields good result, rest are commented out to save perf.
				//Not gonna use G and H for the time being
				const float TentativeG = CalculateTentativeGScore(*CurrentNode, *JumpPoint);

				const float TentativeF = TentativeG + FVector::Dist(ConvertCoordinatesToLocation(*NavVolume, JumpPoint->Coordinates),
				                                                    ConvertCoordinatesToLocation(*NavVolume, EndNode->Coordinates));

				if (JumpPoint->FScore <= TentativeF) continue; //should I check G or F?

				JumpPoint->GScore = TentativeG;
				JumpPoint->FScore = TentativeF;
				ParentMap[JumpPoint] = CurrentNode;

				if (OpenSetCheck.find(Neighbor) == OpenSetCheck.end())
				{
					OpenSet.push(JumpPoint);
					OpenSetCheck.emplace(JumpPoint);
				}
			}
		}
	}
	return false;
}

bool UNavSystemComponent::Jump(const ANavSystemVolume& NavVolume, const NavSystemNode& CurrentNode, const NavSystemNode& Neighbor,
                               AActor* TargetActor, const float& MeshBounds, const ECollisionChannel& CollisionChannel, NavSystemNode** OutJumpPoint)
{
	FIntVector Direction = Neighbor.Coordinates - CurrentNode.Coordinates;

	float MaxSweepDistance = CalculateMaxSweepDistance(NavVolume, CurrentNode, static_cast<FVector>(Direction));

	FHitResult OriginLineResult;
	FVector OriginStart = ConvertCoordinatesToLocation(NavVolume, CurrentNode.Coordinates);
	FVector OriginEnd = OriginStart + static_cast<FVector>(Direction) * MaxSweepDistance;

	if (SweepBoxShape(OriginStart, OriginEnd, CollisionChannel, MeshBounds, OriginLineResult) && OriginLineResult.GetActor() == TargetActor)
	{
		*OutJumpPoint = GetNode(NavVolume, ConvertLocationToCoordinates(NavVolume, TargetActor->GetActorLocation()));
		return true;
	}


#pragma region Direction Calculations

	bool DiagonalMovement = FMath::Abs(Direction.X) + FMath::Abs(Direction.Y) + FMath::Abs(Direction.Z) > 1;
	//Given direction's values are either -1, 0 or 1, so if we take its absolutes (so only 1 and 0)
	//the only way it can be more than 1 is not a straight movement (eg 0,0,1)

	//Need to pass in a rotation as either 0,0,1; 0,1,0 or 1,0,0.
	auto RotateVector2D = [](const FIntVector& VectorToRotate, const float& AngleInDegrees, const FIntVector& Axis)
	{
		const float AngleInRadians = FMath::DegreesToRadians(AngleInDegrees);
		const float CosTheta = FMath::Cos(AngleInRadians);
		const float SinTheta = FMath::Sin(AngleInRadians);

		FVector RotatedVector;

		if (Axis == FIntVector(1, 0, 0)) // Rotate around the X-axis
		{
			RotatedVector.X = VectorToRotate.X;
			RotatedVector.Y = VectorToRotate.Y * CosTheta - VectorToRotate.Z * SinTheta;
			RotatedVector.Z = VectorToRotate.Y * SinTheta + VectorToRotate.Z * CosTheta;
		}
		else if (Axis == FIntVector(0, 1, 0)) // Rotate around the Y-axis
		{
			RotatedVector.X = VectorToRotate.X * CosTheta + VectorToRotate.Z * SinTheta;
			RotatedVector.Y = VectorToRotate.Y;
			RotatedVector.Z = -VectorToRotate.X * SinTheta + VectorToRotate.Z * CosTheta;
		}
		else if (Axis == FIntVector(0, 0, 1)) // Rotate around the Z-axis
		{
			RotatedVector.X = VectorToRotate.X * CosTheta - VectorToRotate.Y * SinTheta;
			RotatedVector.Y = VectorToRotate.X * SinTheta + VectorToRotate.Y * CosTheta;
			RotatedVector.Z = VectorToRotate.Z;
		}
		else
		{
			// Invalid axis or custom axis rotation.
			return RotatedVector = FVector(0, 0, 0);
		}

		return RotatedVector;
	};

	std::vector<FVector> RotatedDirections;
	std::vector<FVector> PossibleStarts;

	if (!DiagonalMovement)
	{
		std::vector<FIntVector> PossibleDirections;
		//Orthogonal + Diagonal Directions - Diagonal can be left out if the agent cannot move diagonally.

		if (Direction.Y == 0 && Direction.Z == 0) //We are moving in X (1 or -1) Direction (forwards or backwards)
		{
			FIntVector Right = FIntVector(0, 1, 0);
			FIntVector Left = FIntVector(0, -1, 0); //While all the opposites need not be to declared. However, this is more readable.
			FIntVector Above = FIntVector(0, 0, 1);
			FIntVector Below = FIntVector(0, 0, -1);

			PossibleDirections.push_back(Right);
			PossibleDirections.push_back(Left);
			PossibleDirections.push_back(Above);
			PossibleDirections.push_back(Below);

			PossibleDirections.push_back(Above + Right);
			PossibleDirections.push_back(Above + Left);
			PossibleDirections.push_back(Below + Right);
			PossibleDirections.push_back(Below + Left);
		}

		else if (Direction.X == 0 && Direction.Z == 0) //We are moving in Y (1 or -1) Direction (right or left)
		{
			FIntVector Left = FIntVector(1, 0, 0);
			FIntVector Right = FIntVector(-1, 0, 0);
			FIntVector Above = FIntVector(0, 0, 1);
			FIntVector Below = FIntVector(0, 0, -1);

			PossibleDirections.push_back(Left);
			PossibleDirections.push_back(Right);
			PossibleDirections.push_back(Above);
			PossibleDirections.push_back(Below);

			PossibleDirections.push_back(Above + Right);
			PossibleDirections.push_back(Above + Left);
			PossibleDirections.push_back(Below + Right);
			PossibleDirections.push_back(Below + Left);
		}

		else if (Direction.X == 0 && Direction.Y == 0) //We are moving in Z (1 or -1) Direction (up or down)
		{
			FIntVector Forward = FIntVector(1, 0, 0);
			FIntVector Backward = FIntVector(-1, 0, 0);
			FIntVector Right = FIntVector(0, 1, 0);
			FIntVector Left = FIntVector(0, -1, 0);

			PossibleDirections.push_back(Forward);
			PossibleDirections.push_back(Backward);
			PossibleDirections.push_back(Right);
			PossibleDirections.push_back(Left);

			PossibleDirections.push_back(Forward + Right);
			PossibleDirections.push_back(Forward + Left);
			PossibleDirections.push_back(Backward + Right);
			PossibleDirections.push_back(Backward + Left);
		}

		for (FIntVector Possibility : PossibleDirections)
		{
			FIntVector toAdd = CurrentNode.Coordinates + Possibility + Direction;
			if (GetNode(NavVolume, toAdd) != nullptr)
			{
				PossibleStarts.push_back(ConvertCoordinatesToLocation(NavVolume, toAdd));
			}
		}
	}

	else //Diagonal Movement
	{
		FVector RotatedVector1 = RotateVector2D(Direction, 45.0f, FIntVector(0, 0, 1));
		FVector RotatedVector2 = RotateVector2D(Direction, -45.0f, FIntVector(0, 0, 1));

		RotatedVector1.Z = 0;
		RotatedVector2.Z = 0;

		FVector RotatedVector1UP = RotatedVector1 + FVector(0, 0, 1);
		FVector RotatedVector1DOWN = RotatedVector1 + FVector(0, 0, -1);

		FVector RotatedVector2UP = RotatedVector2 + FVector(0, 0, 1);
		FVector RotatedVector2DOWN = RotatedVector2 + FVector(0, 0, -1);

		RotatedDirections.push_back(RotatedVector1);
		RotatedDirections.push_back(RotatedVector1UP);
		RotatedDirections.push_back(RotatedVector1DOWN);

		RotatedDirections.push_back(RotatedVector2);
		RotatedDirections.push_back(RotatedVector2DOWN);
		RotatedDirections.push_back(RotatedVector2UP);
	}

#pragma endregion


	FHitResult HitResult;

	if (!DiagonalMovement)
	{
		std::multiset<FHitResult, FHitResultCompare> HitResultsSet;

		for (int32 i = 0; i < PossibleStarts.size(); i++)
		{
			FVector Start = PossibleStarts[i];
			FVector End = Start + static_cast<FVector>(Direction) * MaxSweepDistance;

			if (SweepBoxShape(Start, End, CollisionChannel, MeshBounds, HitResult))
			{
				if (HitResult.Distance > 0 && HitResult.Distance < OriginLineResult.Distance - 1)
				{
					if (HitResult.GetActor() == TargetActor)
					{
						*OutJumpPoint = GetNode(NavVolume, ConvertLocationToCoordinates(NavVolume, TargetActor->GetActorLocation()));
						return true;
					}
					HitResultsSet.emplace(HitResult);
				}
			}
		}


		if (HitResultsSet.size() > 0)
		{
			FHitResult closestHit = *HitResultsSet.begin();

			//TODO World Coords. Adding a tiny vector pointing towards hit object's center because when an object is just on the edge of the grid,
			//the impact point is technically on the grid that's before it by fraction of a cm.

			FVector JumpPointCoords = closestHit.ImpactPoint + (OriginLineResult.TraceStart - closestHit.TraceStart);
			*OutJumpPoint = GetNode(NavVolume, ConvertLocationToCoordinates(NavVolume, JumpPointCoords));
			return true;
		}
	}
	else
	{
		int32 searchIndex = 1;
		while (AreCoordinatesValid(NavVolume, CurrentNode.Coordinates + Direction * (searchIndex + 1)))
		{
			FVector Previous = ConvertCoordinatesToLocation(NavVolume, CurrentNode.Coordinates + Direction * (searchIndex - 1));
			FVector Start = ConvertCoordinatesToLocation(NavVolume, CurrentNode.Coordinates + Direction * searchIndex);

			if (OverlapBoxShape(Start, CollisionChannel, MeshBounds)) return false;

			FVector NextDiagonal = ConvertCoordinatesToLocation(NavVolume, CurrentNode.Coordinates + Direction * (searchIndex + 1));
			if (OverlapBoxShape(NextDiagonal, CollisionChannel, MeshBounds))
			{
				*OutJumpPoint = GetNode(NavVolume, ConvertLocationToCoordinates(NavVolume, NextDiagonal));
				return true;
			}


			for (int32 i = 0; i < RotatedDirections.size(); i++)
			{
				const float RotatedSweep = CalculateMaxSweepDistance(NavVolume, *GetNode(NavVolume, ConvertLocationToCoordinates(NavVolume, Start)),
				                                                     RotatedDirections[i]);
				if (RotatedSweep < 1) continue;

				const FVector End = Start + RotatedDirections[i] * RotatedSweep;

				if (SweepBoxShape(Start, End, CollisionChannel, MeshBounds, HitResult))
				{
					*OutJumpPoint = GetNode(NavVolume, ConvertLocationToCoordinates(NavVolume, Start));
					return true;
				}
			}
			searchIndex++;
		}
	}
	return false;
}


float UNavSystemComponent::CalculateMaxSweepDistance(const ANavSystemVolume& NavVolume, const NavSystemNode& CurrentNode, const FVector& Direction)
{
	const FIntVector GridSize = FIntVector(NavVolume.GetDivisionsX() - 1, NavVolume.GetDivisionsY() - 1, NavVolume.GetDivisionsZ() - 1);
	
	FIntVector MaxDistance = CurrentNode.Coordinates;


	if (Direction.X > 0)
	{
		MaxDistance.X = GridSize.X;
	}
	else if (Direction.X < 0)
	{
		MaxDistance.X = 0;
	}

	if (Direction.Y > 0)
	{
		MaxDistance.Y = GridSize.Y;
	}
	else if (Direction.Y < 0)
	{
		MaxDistance.Y = 0;
	}

	if (Direction.Z > 0)
	{
		MaxDistance.Z = GridSize.Z;
	}
	else if (Direction.Z < 0)
	{
		MaxDistance.Z = 0;
	}

	return (ConvertCoordinatesToLocation(NavVolume, MaxDistance) - ConvertCoordinatesToLocation(NavVolume, CurrentNode.Coordinates)).Length();
}

void UNavSystemComponent::AddNeighbors(const ANavSystemVolume& NavVolume, NavSystemNode* CurrentNode)
{
	auto addNeighborIfValid = [&](const ANavSystemVolume& NavVolumeToCheck, NavSystemNode* Node, const FIntVector& Neighbor_Coordinates)
	{
		// Make sure the neighboring coordinates are valid
		if (AreCoordinatesValid(NavVolumeToCheck, Neighbor_Coordinates))
		{
			int32 sharedAxes = 0;
			if (Node->Coordinates.X == Neighbor_Coordinates.X)
				++sharedAxes;
			if (Node->Coordinates.Y == Neighbor_Coordinates.Y)
				++sharedAxes;
			if (Node->Coordinates.Z == Neighbor_Coordinates.Z)
				++sharedAxes;

			// Only add the neighbor if we share more axes with it than the required min shared neighbor axes and it isn't the same node as us
			if (sharedAxes >= NavVolume.GetMinSharedNeighborAxes() && sharedAxes < 3)
			{
				Node->Neighbors.push_back(GetNode(NavVolume, Neighbor_Coordinates));
			}
		}
	};

	// Extract coordinates from the current node
	const int32 x = CurrentNode->Coordinates.X;
	const int32 y = CurrentNode->Coordinates.Y;
	const int32 z = CurrentNode->Coordinates.Z;


	for (int32 dx = -1; dx <= 1; ++dx)
	{
		for (int32 dy = -1; dy <= 1; ++dy)
		{
			for (int32 dz = -1; dz <= 1; ++dz)
			{
				// Skip the center node (dx=0, dy=0, dz=0)
				if (dx == 0 && dy == 0 && dz == 0)
					continue;

				FIntVector NeighborCoordinates(x + dx, y + dy, z + dz);
				addNeighborIfValid(NavVolume, CurrentNode, NeighborCoordinates);
			}
		}
	}
}

void UNavSystemComponent::ClampCoordinates(const ANavSystemVolume& NavVolume, FIntVector& Coordinates)
{
	Coordinates.X = FMath::Clamp(Coordinates.X, 0, NavVolume.GetDivisionsX() - 1);
	Coordinates.Y = FMath::Clamp(Coordinates.Y, 0, NavVolume.GetDivisionsY() - 1);
	Coordinates.Z = FMath::Clamp(Coordinates.Z, 0, NavVolume.GetDivisionsZ() - 1);
}


bool UNavSystemComponent::AreCoordinatesValid(const ANavSystemVolume& NavVolume, const FIntVector& Coordinates)
{
	return Coordinates.X >= 0 && Coordinates.X < NavVolume.GetDivisionsX() &&
		Coordinates.Y >= 0 && Coordinates.Y < NavVolume.GetDivisionsY() &&
		Coordinates.Z >= 0 && Coordinates.Z < NavVolume.GetDivisionsZ();
}

bool UNavSystemComponent::SweepBoxShape(const FVector& Start, const FVector& End, const ECollisionChannel& CollisionChannel,
                                        const float& BoxColliderSize, FHitResult& OutHitResult) const
{
	const FCollisionShape HitBox = FCollisionShape::MakeBox(FVector(BoxColliderSize));
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(GetOwner());

	return GetWorld()->SweepSingleByChannel(OutHitResult, Start, End, FQuat::Identity, CollisionChannel, HitBox, TraceParams);
}

bool UNavSystemComponent::OverlapBoxShape(const FVector& Location, const ECollisionChannel& CollisionChannel, const float& BoxColliderSize) const
{
	const FCollisionShape HitBox = FCollisionShape::MakeBox(FVector(BoxColliderSize));
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(GetOwner());

	return GetWorld()->OverlapAnyTestByChannel(Location, FQuat::Identity, CollisionChannel, HitBox, TraceParams);
}

NavSystemNode* UNavSystemComponent::GetNode(const ANavSystemVolume& NavVolume, FIntVector Coordinates)
{
	ClampCoordinates(NavVolume, Coordinates);

	const int32 divisionPerLevel = NavVolume.GetDivisionsX() * NavVolume.GetDivisionsY();
	const int32 index = (Coordinates.Z * divisionPerLevel) + (Coordinates.Y * NavVolume.GetDivisionsX()) + Coordinates.X;

	return &NavVolume.GetNodeArray()[index];
}

FIntVector UNavSystemComponent::ConvertLocationToCoordinates(const ANavSystemVolume& NavVolume, const FVector& Location)
{
	FIntVector Coordinates;

	// Convert the location into grid space
	const FVector gridSpaceLocation = UKismetMathLibrary::InverseTransformLocation(NavVolume.GetActorTransform(), Location);

	// Convert the grid space location to a coordinate (x,y,z)
	Coordinates.X = NavVolume.GetDivisionsX() * (gridSpaceLocation.X / NavVolume.GetGridSizeX());
	Coordinates.Y = NavVolume.GetDivisionsY() * (gridSpaceLocation.Y / NavVolume.GetGridSizeY());
	Coordinates.Z = NavVolume.GetDivisionsZ() * (gridSpaceLocation.Z / NavVolume.GetGridSizeZ());

	ClampCoordinates(NavVolume, Coordinates);
	return Coordinates;
}

FVector UNavSystemComponent::ConvertCoordinatesToLocation(const ANavSystemVolume& NavVolume, const FIntVector& Coordinates)
{
	FIntVector ClampedCoordinates(Coordinates);
	ClampCoordinates(NavVolume, ClampedCoordinates);

	// Convert the coordinates into a grid space location
	FVector GridSpaceLocation;
	GridSpaceLocation.X = (ClampedCoordinates.X * NavVolume.GetDivisionSize()) + (NavVolume.GetDivisionSize() * 0.5f);
	GridSpaceLocation.Y = (ClampedCoordinates.Y * NavVolume.GetDivisionSize()) + (NavVolume.GetDivisionSize() * 0.5f);
	GridSpaceLocation.Z = (ClampedCoordinates.Z * NavVolume.GetDivisionSize()) + (NavVolume.GetDivisionSize() * 0.5f);

	// Convert the grid space location into world space
	return UKismetMathLibrary::TransformLocation(NavVolume.GetActorTransform(), GridSpaceLocation);
}
