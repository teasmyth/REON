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
				//
				bool tmp = UKismetSystemLibrary::BoxOverlapActors(GWorld, ConvertCoordinatesToLocation(node->Coordinates),
				                                                  FVector(GetDivisionSize() / 2), filter,
				                                                  nullptr, TArray<AActor*>(), outActors);

				if (tmp)
				{
					//DrawDebugBox(GWorld, ConvertCoordinatesToLocation(node->Coordinates), FVector(GetDivisionSize() / 2.2), FColor::Cyan, false, 30.0f, 0,2.0f);
				}
				//

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

bool ANavigationVolume3D::FindPath(const FVector& start, const FVector& destination, const TArray<TEnumAsByte<EObjectTypeQuery>>& object_types,
                                   const float& meshBounds, UClass* actor_class_filter,
                                   TArray<FVector>& out_path)

{
	bool drawdebugenabled = false;
	for (int32 i = 0; i < GetTotalDivisions(); ++i)
	{
		//Nodes[i].FScore = Nodes[i].GScore = Nodes[i].HScore = MAX_FLT;
	}

	// Create open and closed sets
	std::priority_queue<NavNode*, std::vector<NavNode*>, NodeCompare> openSet;
	std::set<NavNode*> openSetCheck;
	std::set<NavNode*> closedSet;
	std::unordered_map<NavNode*, NavNode*> parentMap; //parent system, to tell where can you come from to a particular node


	//Helper methods
	auto ReconstructPath = [&](const std::unordered_map<NavNode*, NavNode*>& cameFrom, NavNode* current, TArray<FVector>& path)
	{
		// Reconstruct the path from the cameFrom map
		out_path.Empty();

		while (current)
		{
			path.Insert(ConvertCoordinatesToLocation(current->Coordinates), 0);
			current = cameFrom.count(current) ? cameFrom.at(current) : nullptr;
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

	// Initialize start node
	NavNode* startNode = GetNode(ConvertLocationToCoordinates(start));
	NavNode* endNode = GetNode(ConvertLocationToCoordinates(destination));


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
	startNode->HScore = FVector::Dist(ConvertCoordinatesToLocation(startNode->Coordinates), ConvertCoordinatesToLocation(endNode->Coordinates));
	startNode->FScore = startNode->HScore;

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
			return true;
		}

		openSet.pop();
		openSetCheck.erase(current);
		closedSet.insert(current);
		
		if (drawdebugenabled) DrawDebugSphere(GetWorld(), ConvertCoordinatesToLocation(current->Coordinates), meshBounds, 4, FColor::Green,
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
			neighbor->HScore = FVector::Dist(ConvertCoordinatesToLocation(neighbor->Coordinates), ConvertCoordinatesToLocation(endNode->Coordinates));
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

	const int32 divisionPerLevel = DivisionsX * DivisionsY;
	const int32 index = (coordinates.Z * divisionPerLevel) + (coordinates.Y * DivisionsX) + coordinates.X;
	return &Nodes[index];
}
