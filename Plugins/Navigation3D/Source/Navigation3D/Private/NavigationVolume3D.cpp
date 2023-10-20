// Fill out your copyright notice in the Description page of Project Settings.


#include "NavigationVolume3D.h"

#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NavNode.h"
#include <set>
#include <unordered_map>
#include <unordered_set>
#include "DrawDebugHelpers.h"
#include "Algo/Reverse.h"
#include "queue"

static UMaterial* GridMaterial = nullptr;

// Sets default values
ANavigationVolume3D::ANavigationVolume3D()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create the default scene component
	DefaultSceneComponent = CreateDefaultSubobject<USceneComponent>("DefaultSceneComponent");
	SetRootComponent(DefaultSceneComponent);

	// Create the procedural mesh component
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>("ProceduralMesh");
	ProceduralMesh->SetupAttachment(GetRootComponent());
	ProceduralMesh->CastShadow = false;
	ProceduralMesh->SetEnableGravity(false);
	ProceduralMesh->bApplyImpulseOnDamage = false;
	ProceduralMesh->SetGenerateOverlapEvents(false);
	ProceduralMesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	ProceduralMesh->SetCollisionProfileName("NoCollision");
	ProceduralMesh->bHiddenInGame = false;

	// By default, hide the volume while the game is running
	SetActorHiddenInGame(true);

	// Find and save the grid material
	static ConstructorHelpers::FObjectFinder<UMaterial> materialFinder(TEXT("Material'/Navigation3D/M_Grid.M_Grid'"));
	GridMaterial = materialFinder.Object;
}

void ANavigationVolume3D::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Create arrays to store the vertices and the triangles
	TArray<FVector> vertices;
	TArray<int32> triangles;

	// Define variables for the start and end of the line
	FVector start = FVector::ZeroVector;
	FVector end = FVector::ZeroVector;

	// Create the X lines geometry
	for (int32 z = 0; z <= DivisionsZ; ++z)
	{
		start.Z = end.Z = z * DivisionSize;
		for (int32 x = 0; x <= DivisionsX; ++x)
		{
			start.X = end.X = (x * DivisionSize);
			end.Y = GetGridSizeY();

			CreateLine(start, end, FVector::UpVector, vertices, triangles);
		}
	}

	// Reset start and end variables
	start = FVector::ZeroVector;
	end = FVector::ZeroVector;

	// Create the Y lines geometry
	for (int32 z = 0; z <= DivisionsZ; ++z)
	{
		start.Z = end.Z = z * DivisionSize;
		for (int32 y = 0; y <= DivisionsY; ++y)
		{
			start.Y = end.Y = (y * DivisionSize);
			end.X = GetGridSizeX();

			CreateLine(start, end, FVector::UpVector, vertices, triangles);
		}
	}

	// Reset start and end variables
	start = FVector::ZeroVector;
	end = FVector::ZeroVector;

	// Create the Z lines geometry
	for (int32 x = 0; x <= DivisionsX; ++x)
	{
		start.X = end.X = x * DivisionSize;
		for (int32 y = 0; y <= DivisionsY; ++y)
		{
			start.Y = end.Y = (y * DivisionSize);
			end.Z = GetGridSizeZ();

			CreateLine(start, end, FVector::ForwardVector, vertices, triangles);
		}
	}

	// Unused variables that are required to be passed to CreateMeshSection
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	// Add the geometry to the procedural mesh so it will render
	ProceduralMesh->CreateMeshSection(0, vertices, triangles, Normals, UVs, Colors, Tangents, false);

	// Set the material on the procedural mesh so it's color/opacity can be configurable
	UMaterialInstanceDynamic* dynamicMaterialInstance = UMaterialInstanceDynamic::Create(GridMaterial, this);
	dynamicMaterialInstance->SetVectorParameterValue("Color", Color);
	dynamicMaterialInstance->SetScalarParameterValue("Opacity", Color.A);
	ProceduralMesh->SetMaterial(0, dynamicMaterialInstance);
}

// Called when the game starts or when spawned
void ANavigationVolume3D::BeginPlay()
{
	Super::BeginPlay();

	// Allocate nodes used for pathfinding
	Nodes = new NavNode[GetTotalDivisions()];

	// Helper lambda for adding a neighbor
	auto addNeighborIfValid = [&](NavNode* node, const FIntVector& neighbor_coordinates)
	{
		// Make sure the neighboring coordinates are valid
		if (AreCoordinatesValid(neighbor_coordinates))
		{
			int32 sharedAxes = 0;
			if (node->Coordinates.X == neighbor_coordinates.X)
				++sharedAxes;
			if (node->Coordinates.Y == neighbor_coordinates.Y)
				++sharedAxes;
			if (node->Coordinates.Z == neighbor_coordinates.Z)
				++sharedAxes;

			// Only add the neighbor if we share more axes with it then the required min shared neighbor axes and it isn't the same node as us
			if (sharedAxes >= MinSharedNeighborAxes && sharedAxes < 3)
			{
				node->Neighbors.push_back(GetNode(neighbor_coordinates));
			}
		}
	};
	TArray<AActor*> outActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> filter;
	filter.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
	filter.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));

	TArray<AActor*, FDefaultAllocator> ignoreActors;

	// For each node, find its neighbors and assign its coordinates
	for (int32 z = 0; z < DivisionsZ; ++z)
	{
		for (int32 y = 0; y < DivisionsY; ++y)
		{
			for (int32 x = 0; x < DivisionsX; ++x)
			{
				NavNode* node = GetNode(FIntVector(x, y, z));
				node->Coordinates = FIntVector(x, y, z);

				// Above neighbors
				{
					// front
					{
						addNeighborIfValid(node, FIntVector(x + 1, y - 1, z + 1));
						addNeighborIfValid(node, FIntVector(x + 1, y + 0, z + 1));
						addNeighborIfValid(node, FIntVector(x + 1, y + 1, z + 1));
					}
					// middle
					{
						addNeighborIfValid(node, FIntVector(x + 0, y - 1, z + 1));
						addNeighborIfValid(node, FIntVector(x + 0, y + 0, z + 1));
						addNeighborIfValid(node, FIntVector(x + 0, y + 1, z + 1));
					}
					// back
					{
						addNeighborIfValid(node, FIntVector(x - 1, y - 1, z + 1));
						addNeighborIfValid(node, FIntVector(x - 1, y + 0, z + 1));
						addNeighborIfValid(node, FIntVector(x - 1, y + 1, z + 1));
					}
				}

				// Middle neighbors
				{
					// front
					{
						addNeighborIfValid(node, FIntVector(x + 1, y - 1, z + 0));
						addNeighborIfValid(node, FIntVector(x + 1, y + 0, z + 0));
						addNeighborIfValid(node, FIntVector(x + 1, y + 1, z + 0));
					}
					// middle
					{
						addNeighborIfValid(node, FIntVector(x + 0, y - 1, z + 0));
						addNeighborIfValid(node, FIntVector(x + 0, y + 1, z + 0));
					}
					// back
					{
						addNeighborIfValid(node, FIntVector(x - 1, y - 1, z + 0));
						addNeighborIfValid(node, FIntVector(x - 1, y + 0, z + 0));
						addNeighborIfValid(node, FIntVector(x - 1, y + 1, z + 0));
					}
				}

				// Below neighbors
				{
					// front
					{
						addNeighborIfValid(node, FIntVector(x + 1, y - 1, z - 1));
						addNeighborIfValid(node, FIntVector(x + 1, y + 0, z - 1));
						addNeighborIfValid(node, FIntVector(x + 1, y + 1, z - 1));
					}
					// middle
					{
						addNeighborIfValid(node, FIntVector(x + 0, y - 1, z - 1));
						addNeighborIfValid(node, FIntVector(x + 0, y + 0, z - 1));
						addNeighborIfValid(node, FIntVector(x + 0, y + 1, z - 1));
					}
					// back
					{
						addNeighborIfValid(node, FIntVector(x - 1, y - 1, z - 1));
						addNeighborIfValid(node, FIntVector(x - 1, y + 0, z - 1));
						addNeighborIfValid(node, FIntVector(x - 1, y + 1, z - 1));
					}
				}
			}
		}
	}
}

void ANavigationVolume3D::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Delete the nodes
	delete [] Nodes;
	Nodes = nullptr;

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void ANavigationVolume3D::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool ANavigationVolume3D::FindPath(const FVector& start, const FVector& destination, AActor* target,
                                   const TArray<TEnumAsByte<EObjectTypeQuery>>& object_types,
                                   const float& meshBounds, UClass* actor_class_filter,
                                   TArray<FVector>& out_path, const bool& useAStar)

{
	double StartTimeAStar = FPlatformTime::Seconds();

	bool drawdebugenabled = false;
	// for (int32 i = 0; i < GetTotalDivisions(); ++i)
	// {
	// 	  Nodes[i].FScore = Nodes[i].GScore = Nodes[i].HScore = MAX_FLT;
	// }

	// Create open and closed sets
	std::priority_queue<NavNode*, std::vector<NavNode*>, NodeCompare> openSet;
	std::set<NavNode*> openSetCheck; //For checking if something is in open set.
	std::set<NavNode*> closedSet;
	std::unordered_map<NavNode*, NavNode*> parentMap; //parent system, to tell where can you come from to a particular node

#pragma region Helper methods

	auto ReconstructPath = [&](const std::unordered_map<NavNode*, NavNode*>& cameFrom, NavNode* current, TArray<FVector>& path)
	{
		// Reconstruct the path from the cameFrom map
		out_path.Empty();
		TArray<FVector> smoothenedPath;

		while (current)
		{
			path.Insert(ConvertCoordinatesToLocation(current->Coordinates), 0);
			current = cameFrom.count(current) ? cameFrom.at(current) : nullptr;
		}

		if (out_path.Num() > 2)
		{
			ETraceTypeQuery TraceChannel = UEngineTypes::ConvertToTraceType(ECC_Visibility); // Specify the trace channel using ECC_Visibility
			FVector currentPoint = *out_path.begin();
			smoothenedPath.Add(currentPoint);


			//out path num = 9  -> loop max is 8,
			for (int i = 0; i < out_path.Num() - 2; i++)
			{
				FHitResult hitResult;
				bool sphereHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), currentPoint, out_path[i + 1], meshBounds, TraceChannel, false,
				                                                         TArray<AActor*>(),
				                                                         EDrawDebugTrace::Type::None, hitResult, true);

				if (sphereHit)
				{
					currentPoint = out_path[i + 1];
					smoothenedPath.Add(out_path[i + 1]);
				}
			}

			smoothenedPath.Add(out_path.Last());
			out_path = smoothenedPath;
		}
	};

	auto CalculateTentativeGScore = [&](const NavNode& current, const NavNode& neighbor)
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
			return current.GScore + 14; // + AdditionalCosts(current, neighbor); // Consider terrain costs, etc.
		}

		// Horizontal or vertical movement: Add a lower cost (e.g., 10)
		return current.GScore + 10; // + AdditionalCosts(current, neighbor); // Consider terrain costs, etc.
	};

	auto ClearNode = [](NavNode& nodeToClear)
	{
		nodeToClear.FScore = nodeToClear.GScore = nodeToClear.HScore = FLT_MAX;
	};

	auto Cleanup = [&]()
	{
		for (NavNode* item : openSetCheck)
		{
			ClearNode(*item);
		}
		for (NavNode* item : closedSet)
		{
			ClearNode(*item);
		}
	};

	auto IsBlocked = [&](const NavNode* nodeToCheck)
	{
		TArray<AActor*> outActors;
		bool tmp = UKismetSystemLibrary::BoxOverlapActors(GetWorld(), ConvertCoordinatesToLocation(nodeToCheck->Coordinates),
		                                                  /*FVector(GetDivisionSize() / 2.1)*/ FVector(meshBounds) * 1.1f, object_types,
		                                                  actor_class_filter, TArray<AActor*>(), outActors);

		if (tmp && drawdebugenabled)
		{
			DrawDebugBox(GetWorld(), ConvertCoordinatesToLocation(nodeToCheck->Coordinates), FVector(meshBounds), FColor::Red, false, 1, 0,
			             5.0f);
		}
		return tmp;
	};

	auto Heuristics = [](const NavNode* currentNode, const NavNode* endNode)
	{
		return
			FMath::Abs(currentNode->Coordinates.X - endNode->Coordinates.X) +
			FMath::Abs(currentNode->Coordinates.Y - endNode->Coordinates.Y) +
			FMath::Abs(currentNode->Coordinates.Z - endNode->Coordinates.Z);
	};

	//todo doesnt work.
	//D is cost of diagonal movement, default is 14. D2 is cost of cardinal/orthogonal movement, default is 10.
	auto CalculateOctileDistance = [](const NavNode* startNode, const NavNode* targetNode, const int32 D = 14, const int32 D2 = 10)
	{
		int dx = FMath::Abs(targetNode->Coordinates.X - startNode->Coordinates.X);
		int dy = FMath::Abs(targetNode->Coordinates.Y - startNode->Coordinates.Y);
		int dz = FMath::Abs(targetNode->Coordinates.Z - startNode->Coordinates.Z);

		int d1 = FMath::Min3(dx, dy, dz);
		int d2 = FMath::Abs(dx - dy) + FMath::Abs(dy - dz) + FMath::Abs(dz - dx);
		int d3 = 0;

		if (D == 1)
		{
			// The cost of diagonal movement is 1.
			d3 = d1;
		}
		else
		{
			// The cost of diagonal movement is sqrt(3), approximately 1.7321.
			d3 = static_cast<int32>(D * 0.7321 * d1);
		}

		return D * (d1 + d2) + (D2 - 2 * D) * d3;
	};

#pragma  endregion

	// Initialize start node
	NavNode* startNode = GetNode(ConvertLocationToCoordinates(start));

	FIntVector origin = ConvertLocationToCoordinates(destination);
	FIntVector up = origin + FIntVector(0, 0, 1);

	NavNode* endNode = GetNode(origin);

	//bug fix: when player is sharing a grid that's blocked, target the neighbor grid that's free.
	if (IsBlocked(endNode))
	{
		for (NavNode* neighbor : endNode->Neighbors)
		{
			if (!IsBlocked(neighbor))
			{
				endNode = neighbor;
				break;
			}
		}
	}

	startNode->GScore = 0.0f;
	startNode->HScore = 0;
	startNode->FScore = 0;

	if (useAStar)
	{
		openSet.push(startNode);
		openSetCheck.insert(startNode);

		int infiniteloopindex = 0;
		while (!openSet.empty())
		{
			infiniteloopindex++;
			if (infiniteloopindex > 99) //Gotta keep it a bit big in case 
			{
				auto startDebug = startNode;
				auto endDebug = endNode;
				auto openSetDebug = openSet;
				auto closedSetDebug = closedSet;
				UE_LOG(LogTemp, Error, TEXT("The destination is too far to find within time. or Infinite loop :("));
				return false;
			}

			NavNode* current = openSet.top();

			//If the path to the destination is found, reconstruct the path.
			if (current->Coordinates == endNode->Coordinates)
			{
				if (drawdebugenabled)
					DrawDebugSphere(GetWorld(), ConvertCoordinatesToLocation(current->Coordinates), meshBounds, 4, FColor::Blue, false, 2, 0,
					                5.0f);

				ReconstructPath(parentMap, current, out_path);
				Cleanup();
				double EndTimeAStar = FPlatformTime::Seconds();
				double ElapsedTimeAStar = EndTimeAStar - StartTimeAStar;
				UE_LOG(LogTemp, Warning, TEXT(" A* elapsed time: %f seconds"), ElapsedTimeAStar);
				return true;
			}

			openSet.pop();
			openSetCheck.erase(current);
			closedSet.insert(current);

			if (drawdebugenabled)
				DrawDebugSphere(GetWorld(), ConvertCoordinatesToLocation(current->Coordinates), meshBounds, 4, FColor::Green,
				                false, 1, 0, 5.0f);


			for (NavNode* neighbor : current->Neighbors)
			{
				//If blocked or is in closed list (meaning best path found), go on to the next iteration and skip this loop
				if (IsBlocked(neighbor) || closedSet.find(neighbor) != closedSet.end()) continue;

				const float tentativeGScore = CalculateTentativeGScore(*current, *neighbor);

				//checking if hypothetical g score is worse, if so, skip
				if (tentativeGScore > neighbor->GScore) continue;

				parentMap[neighbor] = current;
				neighbor->GScore = tentativeGScore;
				neighbor->HScore = FVector::Dist(ConvertCoordinatesToLocation(neighbor->Coordinates),
				                                 ConvertCoordinatesToLocation(endNode->Coordinates));
				neighbor->FScore = neighbor->GScore + neighbor->HScore;

				//add neighbor if its not in the open list. if its true, it means iterator couldn't find it.
				if (openSetCheck.find(neighbor) == openSetCheck.end())
				{
					openSet.push(neighbor);
					openSetCheck.insert(neighbor);
				}
			}
		}

		UE_LOG(LogTemp, Display, TEXT("Path not found."));
		return false; // No path found
	}
	else
	{
		int infiniteloopindex = 0;

		openSet.push(startNode);
		while (!openSet.empty())
		{
			infiniteloopindex++;
			if (infiniteloopindex > 21) //Gotta keep it a bit big in case 
			{
				return false;
			}
			NavNode* currentNode = openSet.top();
			openSet.pop();
			closedSet.emplace(currentNode);

			if (currentNode->Coordinates == endNode->Coordinates)
			{
				ReconstructPath(parentMap, currentNode, out_path);
				Cleanup();
				//DrawDebugSphere(GetWorld(), ConvertCoordinatesToLocation(currentNode->Coordinates), 30, 4, FColor::Purple, false, 2, 0, 5.0f);

				//UE_LOG(LogTemp, Warning, TEXT("Found path!"));
				double EndTimeAStar = FPlatformTime::Seconds();
				double ElapsedTimeAStar = EndTimeAStar - StartTimeAStar;
				UE_LOG(LogTemp, Warning, TEXT("JPS elapsed time: %f seconds"), ElapsedTimeAStar);
				return true;
			}
			//DrawDebugSphere(GetWorld(), ConvertCoordinatesToLocation(currentNode->Coordinates), 30, 4, FColor::Blue, false, 2, 0, 5.0f);

			for (auto neighbor : currentNode->Neighbors)
			{
				NavNode* jumpPoint = nullptr;
				bool successfulJump = Jump(*currentNode, *neighbor, target, &jumpPoint);
				//DrawDebugBox(GetWorld(), ConvertCoordinatesToLocation(neighbor->Coordinates), FVector(GetDivisionSize() / 2), FColor::Green, true, 1, 0, 2);


				if (successfulJump)
				{
					float tentativeF = currentNode->GScore + 1 + FVector::Dist(ConvertCoordinatesToLocation(jumpPoint->Coordinates),
					                                                            ConvertCoordinatesToLocation(endNode->Coordinates));
					if (jumpPoint->FScore < tentativeF) continue;

					jumpPoint->GScore = currentNode->GScore + 1;
					//jumpPoint->HScore = CalculateOctileDistance(jumpPoint, endNode);
					jumpPoint->HScore = FVector::Dist(ConvertCoordinatesToLocation(jumpPoint->Coordinates),
					                                  ConvertCoordinatesToLocation(endNode->Coordinates));
					jumpPoint->FScore = jumpPoint->GScore + jumpPoint->HScore;
					parentMap[jumpPoint] = currentNode;
					if (openSetCheck.find(neighbor) == openSetCheck.end())
					{
						openSet.push(jumpPoint);
						openSetCheck.emplace(jumpPoint);
						//DrawDebugBox(GetWorld(), ConvertCoordinatesToLocation(jumpPoint->Coordinates), FVector(GetDivisionSize() / 2), FColor::Green, true, 1, 0, 2);
					}
				}
			}
		}
		return false;
	}
}

bool ANavigationVolume3D::Jump(const NavNode& CurrentNode, const NavNode& Neighbor, AActor* Target, NavNode** outJumpPoint)
{
	FIntVector Direction = Neighbor.Coordinates - CurrentNode.Coordinates;
	bool DiagonalMovement = FMath::Abs(Direction.X) + FMath::Abs(Direction.Y) + FMath::Abs(Direction.Z) > 1;
	//Given direction's values are either -1, 0 or 1, so if we take its absolutes (so only 1 and 0)
	//the only way it can be more than 1 is not a straight movement (eg 0,0,1)


#pragma region Direction Calculations
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
			if (GetNode(toAdd) != nullptr)
			{
				PossibleStarts.push_back(ConvertCoordinatesToLocation(toAdd));
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

		//PossibleDirections.push_back(RotatedVector1);
		RotatedDirections.push_back(RotatedVector1);

		// PossibleDirections.push_back(RotatedVector1UP);
		RotatedDirections.push_back(RotatedVector1UP);
		//
		// PossibleDirections.push_back(RotatedVector1DOWN);
		RotatedDirections.push_back(RotatedVector1DOWN);

		//PossibleDirections.push_back(RotatedVector2);
		RotatedDirections.push_back(RotatedVector2);

		// PossibleDirections.push_back(RotatedVector2DOWN);
		RotatedDirections.push_back(RotatedVector2DOWN);
		//
		// PossibleDirections.push_back(RotatedVector2UP);
		RotatedDirections.push_back(RotatedVector2UP);
	}

#pragma endregion

	float MaxSweepDistance = CalculateMaxSweepDistance(CurrentNode, static_cast<FVector>(Direction));
	ECollisionChannel CollisionChannel = ECC_Visibility;

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this); // Ignore the agent itself.
	//FCollisionShape BoxCollider = FCollisionShape::MakeBox(FVector(GetDivisionSize() / 2.1f)); //2.1 in case the edges count as collision


	FHitResult OriginLineResult;
	FVector OriginStart = ConvertCoordinatesToLocation(CurrentNode.Coordinates + Direction);
	FVector OriginEnd = OriginStart + static_cast<FVector>(Direction) * MaxSweepDistance;
	//DrawDebugLine(GetWorld(), OriginStart, OriginEnd, FColor::Blue, true, 5, 0, 5);

	bool originHit = GetWorld()->LineTraceSingleByChannel(OriginLineResult, OriginStart, OriginEnd, ECC_GameTraceChannel1, TraceParams);

	//UE_LOG(LogTemp, Error, TEXT("%s"), *Target->GetActorNameOrLabel());
	if (originHit && OriginLineResult.GetActor() == Target)
	{
		*outJumpPoint = GetNode(ConvertLocationToCoordinates(Target->GetActorLocation()));
		return true;
	}


	if (!DiagonalMovement)
	{
		struct HitResultCompare
		{
			bool operator()(const FHitResult& left, const FHitResult& right) const
			{
				return left.Distance < right.Distance;
			}
		};

		std::multiset<FHitResult, HitResultCompare> HitResultsSet;

		for (int32 i = 0; i < PossibleStarts.size(); i++)
		{
			FHitResult HitResult;
			FVector Start = PossibleStarts[i];
			FVector End = Start + static_cast<FVector>(Direction) * MaxSweepDistance;

			//DrawDebugLine(GetWorld(), Start, End, FColor::Red, true, 1, 0, 5);
			//bool bHit = GetWorld()->SweepSingleByChannel(HitResult, Start, End, FQuat::Identity, ECC_GameTraceChannel1, BoxCollider, TraceParams);
			bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_GameTraceChannel1, TraceParams);

			if (HitResult.GetActor() == Target)
			{
				*outJumpPoint = GetNode(ConvertLocationToCoordinates(Target->GetActorLocation()));
				return true;
			}

			if (bHit && HitResult.Distance > 0 && HitResult.Distance < OriginLineResult.Distance - 1)
			{
				HitResultsSet.emplace(HitResult);
			}
		}

		if (HitResultsSet.size() > 0)
		{
			FHitResult hit = *HitResultsSet.begin();

			//World Coords. Adding a tiny vector pointing towards hit object's center because when an object is just on the edge of the grid,
			//the impact point is technically on the grid that's before it by fraction of a cm.
			FVector dir = (OriginLineResult.TraceStart - hit.TraceStart);
			FVector precisionCorrection = (hit.GetActor()->GetActorLocation() - hit.ImpactPoint).GetSafeNormal();
			FVector jumpPointCoords = hit.ImpactPoint + dir;
			//DrawDebugSphere(GetWorld(), hit.ImpactPoint, 10, 4, FColor::Green, false, 2, 0, 5.0f);

			*outJumpPoint = GetNode(ConvertLocationToCoordinates(jumpPointCoords));
			auto tmp = GetNode(ConvertLocationToCoordinates(jumpPointCoords));
			//DrawDebugSphere(GetWorld(), ConvertCoordinatesToLocation(tmp->Coordinates), 30, 4, FColor::Blue, false, 2, 0, 5.0f);
			return true;
		}
	}
	else
	{
		int32 searchIndex = 1;
		bool coordsValid = true;
		while (coordsValid)
		{
			FVector Previous = ConvertCoordinatesToLocation(CurrentNode.Coordinates + Direction * (searchIndex - 1));
			FVector Start = ConvertCoordinatesToLocation(CurrentNode.Coordinates + Direction * searchIndex);
			FVector NextDiagonal = ConvertCoordinatesToLocation(CurrentNode.Coordinates + Direction * (searchIndex + 1));
			coordsValid = AreCoordinatesValid(CurrentNode.Coordinates + Direction * (searchIndex + 1)); //Nothing to check next loop. End

			FHitResult CurrentHitResult;
			FHitResult NextHitResult;

			bool currentHit = GetWorld()->LineTraceSingleByChannel(CurrentHitResult, Previous, Start, ECC_GameTraceChannel1, TraceParams);
			bool nextHit = GetWorld()->LineTraceSingleByChannel(NextHitResult, Start, NextDiagonal, ECC_GameTraceChannel1, TraceParams);

			if (currentHit) return false;

			if (nextHit)
			{
				*outJumpPoint = GetNode(ConvertLocationToCoordinates(NextDiagonal));
				return true;
			}


			for (int32 i = 0; i < RotatedDirections.size(); i++)
			{
				float RotatedSweep = CalculateMaxSweepDistance(*GetNode(ConvertLocationToCoordinates(Start)), RotatedDirections[i]);
				FHitResult HitResult;
				const float actualSweep = CalculateMaxSweepDistance(*GetNode(ConvertLocationToCoordinates(Start)), RotatedDirections[i]);
				FVector End = Start + RotatedDirections[i] * actualSweep;

				bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_GameTraceChannel1, TraceParams);
				//DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2, 0, 2);

				if (bHit)
				{
					FVector dir = (OriginLineResult.TraceStart - HitResult.TraceStart); //World Coords
					FVector jumpPointCoords = HitResult.ImpactPoint + dir;
					//DrawDebugSphere(GetWorld(), hit.ImpactPoint, 10, 4, FColor::Green, false, 2, 0, 5.0f);
					*outJumpPoint = GetNode(ConvertLocationToCoordinates(Start));
					//DrawDebugBox(GetWorld(), HitResult.ImpactPoint, FVector(GetDivisionSize() / 2), FColor::Green, true, 1, 0, 2);
					return true;
				}
			}
			searchIndex++;
		}
	}
	return false;
}

float ANavigationVolume3D::CalculateMaxSweepDistance(const NavNode& CurrentNode, const FVector& Direction)
{
	// Define the size of your 3D navigation grid (GridWidth, GridHeight, and GridDepth)
	FIntVector GridSize = FIntVector(GetDivisionsX() - 1, GetDivisionsY() - 1, GetDivisionsZ() - 1);

	// Calculate the maximum possible distance in the specified direction
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

	return (ConvertCoordinatesToLocation(MaxDistance) - ConvertCoordinatesToLocation(CurrentNode.Coordinates)).Length();
}


FIntVector ANavigationVolume3D::ConvertLocationToCoordinates(const FVector& location)
{
	FIntVector coordinates;

	// Convert the location into grid space
	const FVector gridSpaceLocation = UKismetMathLibrary::InverseTransformLocation(GetActorTransform(), location);

	// Convert the grid space location to a coordinate (x,y,z)
	coordinates.X = DivisionsX * (gridSpaceLocation.X / GetGridSizeX());
	coordinates.Y = DivisionsY * (gridSpaceLocation.Y / GetGridSizeY());
	coordinates.Z = DivisionsZ * (gridSpaceLocation.Z / GetGridSizeZ());

	ClampCoordinates(coordinates);
	return coordinates;
}

FVector ANavigationVolume3D::ConvertCoordinatesToLocation(const FIntVector& coordinates)
{
	FIntVector clampedCoordinates(coordinates);
	ClampCoordinates(clampedCoordinates);

	// Convert the coordinates into a grid space location
	FVector gridSpaceLocation = FVector::ZeroVector;
	gridSpaceLocation.X = (clampedCoordinates.X * DivisionSize) + (DivisionSize * 0.5f);
	gridSpaceLocation.Y = (clampedCoordinates.Y * DivisionSize) + (DivisionSize * 0.5f);
	gridSpaceLocation.Z = (clampedCoordinates.Z * DivisionSize) + (DivisionSize * 0.5f);

	// Convert the grid space location into world space
	return UKismetMathLibrary::TransformLocation(GetActorTransform(), gridSpaceLocation);
}

void ANavigationVolume3D::CreateLine(const FVector& start, const FVector& end, const FVector& normal, TArray<FVector>& vertices,
                                     TArray<int32>& triangles)
{
	// Calculate the half line thickness and the thickness direction
	float halfLineThickness = LineThickness / 2.0f;
	FVector line = end - start;
	line.Normalize();

	// 0--------------------------1
	// |          line		      |
	// 2--------------------------3

	auto createFlatLine = [&](const FVector& thicknessDirection)
	{
		// Top triangle
		triangles.Add(vertices.Num() + 2);
		triangles.Add(vertices.Num() + 1);
		triangles.Add(vertices.Num() + 0);

		// Bottom triangle
		triangles.Add(vertices.Num() + 2);
		triangles.Add(vertices.Num() + 3);
		triangles.Add(vertices.Num() + 1);

		// Vertex 0
		vertices.Add(start + (thicknessDirection * halfLineThickness));
		// Vertex 1
		vertices.Add(end + (thicknessDirection * halfLineThickness));
		// Vertex 2
		vertices.Add(start - (thicknessDirection * halfLineThickness));
		// Vertex 3
		vertices.Add(end - (thicknessDirection * halfLineThickness));
	};

	FVector direction1 = UKismetMathLibrary::Cross_VectorVector(line, normal);
	FVector direction2 = UKismetMathLibrary::Cross_VectorVector(line, direction1);

	createFlatLine(direction1);
	createFlatLine(direction2);
}

bool ANavigationVolume3D::AreCoordinatesValid(const FIntVector& coordinates) const
{
	return coordinates.X >= 0 && coordinates.X < DivisionsX &&
		coordinates.Y >= 0 && coordinates.Y < DivisionsY &&
		coordinates.Z >= 0 && coordinates.Z < DivisionsZ;
}

void ANavigationVolume3D::ClampCoordinates(FIntVector& coordinates) const
{
	coordinates.X = FMath::Clamp(coordinates.X, 0, DivisionsX - 1);
	coordinates.Y = FMath::Clamp(coordinates.Y, 0, DivisionsY - 1);
	coordinates.Z = FMath::Clamp(coordinates.Z, 0, DivisionsZ - 1);
}


NavNode* ANavigationVolume3D::GetNode(FIntVector coordinates) const
{
	ClampCoordinates(coordinates);

	if (!AreCoordinatesValid(coordinates)) return nullptr;

	const int32 divisionPerLevel = DivisionsX * DivisionsY;
	const int32 index = (coordinates.Z * divisionPerLevel) + (coordinates.Y * DivisionsX) + coordinates.X;

	return &Nodes[index];
}
