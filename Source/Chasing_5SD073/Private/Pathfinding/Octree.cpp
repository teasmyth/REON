// Fill out your copyright notice in the Description page of Project Settings.
#include "Pathfinding/Octree.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"


AOctree::AOctree()
{
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
	ProceduralMesh->CanCharacterStepUpOn = ECB_No;
	ProceduralMesh->SetCollisionProfileName("NoCollision");
	ProceduralMesh->bHiddenInGame = false;
	ProceduralMesh->bUseComplexAsSimpleCollision = false;

	ProceduralMesh->SetMaterial(0, nullptr);


	static ConstructorHelpers::FObjectFinder<UMaterial> MaterialFinder(TEXT("Material'/Game/Materials/Octree/M_OctreeVisual.M_OctreeVisual'"));
	OctreeMaterial = MaterialFinder.Object;
}

void AOctree::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// In Tick method or wherever you want to record frame times, calculate frame time
}

void AOctree::BeginPlay()
{
	Super::BeginPlay();
	//PreviousNextLocation = FVector::ZeroVector;

	//Defining the colliders borders
	const TArray ConvexVerts = {
		FVector(GetOctreeSizeX(), 0, GetOctreeSizeZ()),
		FVector(0, 0, GetOctreeSizeZ()),
		FVector(0, 0, 0),
		FVector(GetOctreeSizeX(), 0, 0),
		FVector(GetOctreeSizeX(), GetOctreeSizeY(), GetOctreeSizeZ()),
		FVector(0, GetOctreeSizeY(), GetOctreeSizeZ()),
		FVector(0, GetOctreeSizeY(), 0),
		FVector(GetOctreeSizeX(), GetOctreeSizeY(), 0)
	};

	ProceduralMesh->ClearCollisionConvexMeshes();
	ProceduralMesh->AddCollisionConvexMesh(ConvexVerts);

	auto TurnTimeToNormalAsync = Async(EAsyncExecution::Thread, [&]()
	{
		// if (ShowGridAfterCalculation)
		// {
		// 	ShowGrid();
		// }
		SetUpOctreesAsync();
		IsSetup = true;
	});
}

void AOctree::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);


	for (const auto RootNode : RootNodes)
	{
		delete RootNode;
	}
	delete NavigationGraph;
}

void AOctree::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (AutoEncapsulateObjects)
	{
		// Choose the appropriate base class based on your requirements
		UClass* ActorBaseClass = AActor::StaticClass();

		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ActorBaseClass, Actors);

		FBox EnclosingBox = Actors[0]->GetComponentsBoundingBox();

		// Iterate over all actors to find the bounding box that encapsulates all positions
		for (const AActor* Actor : Actors)
		{
			if (Actor && !ActorToIgnore.Contains(Actor) && Actor->GetRootComponent() && Actor->GetRootComponent()->GetCollisionObjectType() ==
				CollisionChannel)
			{
				EnclosingBox += Actor->GetComponentsBoundingBox();
			}
		}

		SingleVolumeSize = FMath::Max3(EnclosingBox.GetSize().X, EnclosingBox.GetSize().Y, EnclosingBox.GetSize().Z);
		SetActorLocation(EnclosingBox.Min);
		ExpandVolumeXAxis = ExpandVolumeYAxis = ExpandVolumeZAxis = 1;
	}

	float Power = 0;
	while (MinNodeSize * FMath::Pow(2, Power) < SingleVolumeSize)
	{
		Power++;
	}
	const int PrevSingleVolSize = SingleVolumeSize;
	SingleVolumeSize = MinNodeSize * FMath::Pow(2, Power);
	if (SingleVolumeSize != PrevSingleVolSize && GEngine && !AutoEncapsulateObjects)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, "Volume Size rounded up to be a perfect power of 2 of Min Size");
	}


	if (OctreeMaterial != nullptr)
	{
		DrawOctreeBorders();
	}
}

#pragma region Drawing

void AOctree::CreateCubeMesh(const FVector& Corner1, const FVector& Corner2, const FVector& Corner3, const FVector& Corner4, const FVector& Corner5,
                             const FVector& Corner6, const FVector& Corner7, const FVector& Corner8)
{
	// Define vertices for the cube
	TArray<FVector> Vertices = {
		Corner1, Corner2, Corner3, Corner4, // Front face
		Corner5, Corner6, Corner7, Corner8 // Back face
	};

	// Define triangles (index references to vertices)
	const TArray<int32> Triangles = {
		0, 1, 2, 0, 2, 3, // Front face
		5, 4, 7, 5, 7, 6, // Back face
		4, 0, 3, 4, 3, 7, // Left face
		1, 5, 6, 1, 6, 2, // Right face
		3, 2, 6, 3, 6, 7, // Top face
		4, 5, 1, 4, 1, 0 // Bottom face
	};

	// Define normals for each vertex
	TArray<FVector> Normals;
	for (int32 i = 0; i < Vertices.Num(); ++i)
	{
		Normals.Add(Vertices[i].GetSafeNormal());
	}

	// Define UVs for each vertex
	const TArray<FVector2D> UVs = {
		FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1), FVector2D(0, 1), // Front face
		FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1), FVector2D(0, 1) // Back face
	};

	const TArray<FColor> Colors;
	const TArray<FProcMeshTangent> Tangents;

	// Set the data in the procedural mesh component
	//MeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);


	// Set the material on the procedural mesh so it's color/opacity can be configurable
	UMaterialInstanceDynamic* DynamicMaterialInstance = UMaterialInstanceDynamic::Create(OctreeMaterial, this);
	DynamicMaterialInstance->SetVectorParameterValue("Color", Color);
	DynamicMaterialInstance->SetScalarParameterValue("Opacity", Opacity);
	ProceduralMesh->SetMaterial(0, DynamicMaterialInstance);
}

void AOctree::DrawOctreeBorders()
{
	if (FillBordersOfOctree)
	{
		CreateCubeMesh(
			FVector(GetOctreeSizeX(), 0, GetOctreeSizeZ()),
			FVector(0, 0, GetOctreeSizeZ()),
			FVector(0, 0, 0),
			FVector(GetOctreeSizeX(), 0, 0),
			FVector(GetOctreeSizeX(), GetOctreeSizeY(), GetOctreeSizeZ()),
			FVector(0, GetOctreeSizeY(), GetOctreeSizeZ()),
			FVector(0, GetOctreeSizeY(), 0),
			FVector(GetOctreeSizeX(), GetOctreeSizeY(), 0)
		);
		return;
	}

	// Create arrays to store the vertices and the triangles
	TArray<FVector> Vertices;
	TArray<int32> Triangles;

	// Define variables for the start and end of the line
	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;

	for (int32 y = 0; y <= GetOctreeSizeY(); y += GetOctreeSizeY())
	{
		for (int32 z = 0; z <= GetOctreeSizeZ(); z += GetOctreeSizeZ())
		{
			Start = FVector(0, y, z);
			End = FVector(GetOctreeSizeX(), y, z);
			CreateLine(Start, End, FVector::UpVector, Vertices, Triangles, LineThickness);
		}
	}

	for (int32 x = 0; x <= GetOctreeSizeX(); x += GetOctreeSizeX())
	{
		for (int32 z = 0; z <= GetOctreeSizeZ(); z += GetOctreeSizeZ())
		{
			Start = FVector(x, 0, z);
			End = FVector(x, GetOctreeSizeY(), z);
			CreateLine(Start, End, FVector::UpVector, Vertices, Triangles, LineThickness);
		}
	}

	for (int32 x = 0; x <= GetOctreeSizeX(); x += GetOctreeSizeX())
	{
		for (int32 y = 0; y <= GetOctreeSizeY(); y += GetOctreeSizeY())
		{
			Start = FVector(x, y, 0);
			End = FVector(x, y, GetOctreeSizeZ());
			CreateLine(Start, End, FVector::ForwardVector, Vertices, Triangles, LineThickness);
		}
	}

	// Unused variables that are required to be passed to CreateMeshSection
	const TArray<FVector> Normals;
	const TArray<FVector2D> UVs;
	const TArray<FColor> Colors;
	const TArray<FProcMeshTangent> Tangents;

	// Add the geometry to the procedural mesh so it will render
	ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);

	// Set the material on the procedural mesh so it's color/opacity can be configurable
	UMaterialInstanceDynamic* DynamicMaterialInstance = UMaterialInstanceDynamic::Create(OctreeMaterial, this);
	DynamicMaterialInstance->SetVectorParameterValue("Color", FColor::Yellow);
	DynamicMaterialInstance->SetScalarParameterValue("Opacity", 0.5f);
	ProceduralMesh->SetMaterial(0, DynamicMaterialInstance);
}

void AOctree::CreateLine(const FVector& Start, const FVector& End, const FVector& Normal, TArray<FVector>& Vertices, TArray<int32>& Triangles,
                         const float& LineThickness)
{
	// Calculate the half line thickness and the thickness direction
	const float HalfLineThickness = LineThickness / 2.0f;
	FVector Line = End - Start;
	Line.Normalize();

	// 0--------------------------1
	// |          line		      |
	// 2--------------------------3

	auto CreateFlatLine = [&](const FVector& ThicknessDirection)
	{
		// Top triangle
		Triangles.Add(Vertices.Num() + 2);
		Triangles.Add(Vertices.Num() + 1);
		Triangles.Add(Vertices.Num() + 0);

		// Bottom triangle
		Triangles.Add(Vertices.Num() + 2);
		Triangles.Add(Vertices.Num() + 3);
		Triangles.Add(Vertices.Num() + 1);

		// Vertex 0
		Vertices.Add(Start + (ThicknessDirection * HalfLineThickness));
		// Vertex 1
		Vertices.Add(End + (ThicknessDirection * HalfLineThickness));
		// Vertex 2
		Vertices.Add(Start - (ThicknessDirection * HalfLineThickness));
		// Vertex 3
		Vertices.Add(End - (ThicknessDirection * HalfLineThickness));
	};

	const FVector Direction1 = UKismetMathLibrary::Cross_VectorVector(Line, Normal);
	const FVector Direction2 = UKismetMathLibrary::Cross_VectorVector(Line, Direction1);

	CreateFlatLine(Direction1);
	CreateFlatLine(Direction2);
}

void AOctree::ShowGrid()
{
	// Create arrays to store the vertices and the triangles
	TArray<FVector> Vertices;
	TArray<int32> Triangles;

	for (const auto Node : NavigationGraph->Nodes)
	{
		CreateLine(Node->NodeBounds.Min, FVector(Node->NodeBounds.Max.X, Node->NodeBounds.Min.Y, Node->NodeBounds.Min.Z), FVector::UpVector, Vertices,
		           Triangles, LineThickness);
		CreateLine(Node->NodeBounds.Min, FVector(Node->NodeBounds.Min.X, Node->NodeBounds.Max.Y, Node->NodeBounds.Min.Z), FVector::UpVector, Vertices,
		           Triangles, LineThickness);
		CreateLine(Node->NodeBounds.Min, FVector(Node->NodeBounds.Min.X, Node->NodeBounds.Min.Y, Node->NodeBounds.Max.Z), FVector::ForwardVector,
		           Vertices, Triangles, LineThickness);
		CreateLine(Node->NodeBounds.Max, FVector(Node->NodeBounds.Min.X, Node->NodeBounds.Max.Y, Node->NodeBounds.Max.Z), FVector::UpVector, Vertices,
		           Triangles, LineThickness);
		CreateLine(Node->NodeBounds.Max, FVector(Node->NodeBounds.Max.X, Node->NodeBounds.Min.Y, Node->NodeBounds.Max.Z), FVector::UpVector, Vertices,
		           Triangles, LineThickness);
		CreateLine(Node->NodeBounds.Max, FVector(Node->NodeBounds.Max.X, Node->NodeBounds.Max.Y, Node->NodeBounds.Min.Z), FVector::UpVector, Vertices,
		           Triangles, LineThickness);
		//CreateLine(Start, End, FVector::UpVector, Vertices, Triangles, LineThickness);
		//CreateLine(Start, End, FVector::UpVector, Vertices, Triangles, LineThickness);
		//TODO
	}

	// Unused variables that are required to be passed to CreateMeshSection
	const TArray<FVector> Normals;
	const TArray<FVector2D> UVs;
	const TArray<FColor> Colors;
	const TArray<FProcMeshTangent> Tangents;

	// Add the geometry to the procedural mesh so it will render
	ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);

	// Set the material on the procedural mesh so it's color/opacity can be configurable
	UMaterialInstanceDynamic* DynamicMaterialInstance = UMaterialInstanceDynamic::Create(OctreeMaterial, this);
	DynamicMaterialInstance->SetVectorParameterValue("Color", FColor::Yellow);
	DynamicMaterialInstance->SetScalarParameterValue("Opacity", 0.5f);
	ProceduralMesh->SetMaterial(0, DynamicMaterialInstance);
}

#pragma endregion

void AOctree::SetUpOctreesAsync()
{
	RootNodes.Empty();

	NavigationGraph = new OctreeGraph();

	for (int X = 0; X < ExpandVolumeXAxis; X++)
	{
		for (int Y = 0; Y < ExpandVolumeYAxis; Y++)
		{
			for (int Z = 0; Z < ExpandVolumeZAxis; Z++)
			{
				const FVector Offset = FVector(X * SingleVolumeSize, Y * SingleVolumeSize, Z * SingleVolumeSize);
				MakeOctree(GetActorLocation() + Offset);
			}
		}
	}

	NavigationGraph->ConnectNodes();
}

void AOctree::MakeOctree(const FVector& Origin)
{
	const FVector Size = FVector(SingleVolumeSize);
	const FBox Bounds = FBox(Origin, Origin + Size);
	OctreeNode* NewRootNode = new OctreeNode(Bounds, nullptr);
	RootNodes.Add(NewRootNode);
	NavigationGraph->AddRootNode(NewRootNode);


	if (!UsePhysicsOverlap)
	{
		TArray<FOverlapResult> Result;
		FCollisionQueryParams TraceParams;
		TraceParams.AddIgnoredActor(this);

		if (!ActorToIgnore.IsEmpty())
		{
			for (const auto Actors : ActorToIgnore)
			{
				TraceParams.AddIgnoredActor(Actors);
			}
		}

		GetWorld()->OverlapMultiByChannel(Result, Bounds.GetCenter(), FQuat::Identity, CollisionChannel, FCollisionShape::MakeBox(Size / 2.0f),
		                                  TraceParams);
		AddObjects(Result, NewRootNode);
	}
	else
	{
		NewRootNode->DivideNode(this, MinNodeSize, GetWorld(), false);
	}

	GetEmptyNodes(NewRootNode);
	AdjustChildNodes(NewRootNode);
}

void AOctree::AddObjects(TArray<FOverlapResult> FoundObjects, OctreeNode* RootN) const
{
	if (FoundObjects.IsEmpty())
	{
		return;
	}

	for (const auto& Hit : FoundObjects)
	{
		RootN->DivideNode(Hit.GetActor(), MinNodeSize, GetWorld(), true);
	}
}

void AOctree::GetEmptyNodes(OctreeNode* Node) const
{
	if (Node == nullptr)
	{
		return;
	}

	TArray<OctreeNode*> NodeList;
	NodeList.Add(Node);

	for (int i = 0; i < NodeList.Num(); i++)
	{
		if (NodeList[i] == nullptr)
		{
			continue;
		}

		if (NodeList[i]->ChildrenOctreeNodes.IsEmpty())
		{
			if (!NodeList[i]->Occupied) //NodeList[i]->ContainedActors.IsEmpty())
			{
				NavigationGraph->AddNode(NodeList[i]);
				//DrawDebugBox(GetWorld(), NodeList[i]->NodeBounds.GetCenter(), NodeList[i]->NodeBounds.GetExtent(), FColor::Green, true, 10.0f);
			}
			else
			{
				const int Index = NodeList[i]->Parent->ChildrenOctreeNodes.Find(NodeList[i]);
				NodeList[i]->Parent->ChildrenOctreeNodes[Index] = nullptr;
				delete NodeList[i];
				//I adjust the null pointers in AdjustChildNodes(). Here I should not touch it while it is iterating through it.
			}
		}
		else
		{
			for (const auto Child : NodeList[i]->ChildrenOctreeNodes)
			{
				NodeList.Add(Child);
			}
		}
	}
}

void AOctree::AdjustChildNodes(OctreeNode* Node)
{
	if (Node == nullptr)
	{
		return;
	}

	TArray<OctreeNode*> NodeList;
	NodeList.Add(Node);

	for (int i = 0; i < NodeList.Num(); i++)
	{
		TArray<OctreeNode*> CleanedChildNodes;
		for (auto Child : NodeList[i]->ChildrenOctreeNodes)
		{
			if (Child != nullptr)
			{
				CleanedChildNodes.Add(Child);
			}
		}

		NodeList[i]->ChildrenOctreeNodes = CleanedChildNodes;

		for (const auto Child : CleanedChildNodes)
		{
			NodeList.Add(Child);
		}
	}
}


bool AOctree::GetAStarPath(const AActor* Agent, const FVector& End, FVector& OutNextLocation)
{
	if (!IsSetup)
	{
		return false;
	}

	TArray<FVector> OutPath;
	const FVector Start = Agent->GetActorLocation();

	if (NavigationGraph->OctreeAStar(Start, End, OutPath))
	{
		if (OutPath.Num() < 3)
		{
			OutNextLocation = OutPath[0];
			return true;
		}

		//Path smoothing. If the agent can skip a path point because it wouldn't collide, it should (skip). This ensures a more natural looking movement.
		FHitResult Hit;
		FCollisionShape ColShape = FCollisionShape::MakeSphere(AgentMeshHalfSize);
		FCollisionQueryParams TraceParams;
		TraceParams.AddIgnoredActor(Agent);

		for (int i = 1; i < OutPath.Num(); i++)
		{
			if (!GetWorld()->SweepSingleByChannel(Hit, Start, OutPath[i], FQuat::Identity, CollisionChannel, ColShape, TraceParams))
			{
				continue;
			}

			OutNextLocation = OutPath[i - 1];
			return true;
		}

		//If we are here then there was no path smoothing necessary.
		OutNextLocation = OutPath.Last();
		return true;
	}

	//Couldn't find path, using previous location. Not finding path usually happens when we are moving through a space that is considered occupied
	//However, the path smoothing deems it empty (which is accurate). In those moments, we cannot find a path because we cannot find a node.
	//In that case, we can use previous location and 'drift' until we are once again inside an OctreeNode.
	return false;
}

bool AOctree::GetAStarPathToTarget(const AActor* Agent, const AActor* End, FVector& NextLocation)
{
	return false;
}

void AOctree::GetAStarPathAsyncToLocation(const AActor* Agent, const FVector& Target, FVector& OutNextDirection)
{
	const FVector Start = Agent->GetActorLocation();
	const FVector End = Target;

	if (FVector::Distance(Start, End) <= 20.0f || !IsSetup || AgentMeshHalfSize == 0) //Not meaningful enough to be a public variable, what do I do.
	{
		OutNextDirection = FVector::ZeroVector;
		return;
	}

	// Check if pathfinding is already in progress or if the thread is joinable
	if (IsPathfindingInProgress || (PathfindingThread && PathfindingThread->joinable()))
	{
		OutNextDirection = (PreviousNextLocation - Start).GetSafeNormal();;
		return; // Pathfinding is already in progress, no need to start again
	}

	/*
	 *End and Agent are captured by value, so the thread has its own copy of these variables.
	 *This ensures that the thread can safely use these variables even if they are modified or destroyed in the Blueprint.
	 */
	FVector EndCopy = End;
	const AActor* AgentCopy = Agent;


	PathfindingThread = std::make_unique<std::thread>([&, AgentCopy, EndCopy]()
	{
		std::lock_guard Lock(PathfindingMutex);

		IsPathfindingInProgress = true;
		FVector LocalNextLocation = PreviousNextLocation;

		if (!GetAStarPath(AgentCopy, EndCopy, LocalNextLocation))
		{
			GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, "No path found. Using Previous");
			/*if (PreviousNextLocation == FVector::ZeroVector)
			{
				OutNextDirection = (EndCopy - AgentCopy->GetActorLocation()).GetSafeNormal();
				IsPathfindingInProgress = false;
				return;
			}*/
			OutNextDirection = (EndCopy - AgentCopy->GetActorLocation()).GetSafeNormal();
			//OutNextDirection = (PreviousNextLocation - AgentCopy->GetActorLocation()).GetSafeNormal();
			IsPathfindingInProgress = false;
			return;
		}

		PreviousNextLocation = LocalNextLocation;
		OutNextDirection = (LocalNextLocation - AgentCopy->GetActorLocation()).GetSafeNormal();
		IsPathfindingInProgress = false;
	});


	PathfindingThread->detach();
}

void AOctree::GetAStarPathAsyncToTarget(const AActor* Agent, const AActor* Target, FVector& OutNextLocation)
{
	GetAStarPathAsyncToLocation(Agent, Target->GetActorLocation(), OutNextLocation);
}
