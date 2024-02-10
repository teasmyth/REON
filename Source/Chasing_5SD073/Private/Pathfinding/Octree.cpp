// Fill out your copyright notice in the Description page of Project Settings.


#include "Pathfinding/Octree.h"
#include "ProceduralMeshComponent.h"
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
}

void AOctree::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// In Tick method or wherever you want to record frame times, calculate frame time

	TotalFrameTime += GetWorld()->GetDeltaSeconds();
	NumFrames++;
}

void AOctree::BeginPlay()
{
	Super::BeginPlay();

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

	/*
	auto TurnTimeToNormalAsync = Async(EAsyncExecution::Thread, [&]()
	{
		
	});
	*/
	SetUpOctreesAsync();
	IsSetup = true;
	TotalFrameTime = 0.0f;
}

void AOctree::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	for (const auto RootNode : RootNodes)
	{
		delete RootNode;
	}
	delete NavigationGraph;

	double AverageFrameTime = NumFrames / TotalFrameTime;
	UE_LOG(LogTemp, Warning, TEXT("Average Frame Time: %f seconds"), AverageFrameTime);
}

void AOctree::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
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

	//const double StartTime = FPlatformTime::Seconds();
	NavigationGraph->ConnectNodes();
	//UE_LOG(LogTemp, Warning, TEXT("Connecting neighboring graph nodes took this many seconds: %lf"), FPlatformTime::Seconds() - StartTime);
	UE_LOG(LogTemp, Warning, TEXT("Total Nodes without check inside: %i"), NavigationGraph->Nodes.Num());
}

void AOctree::MakeOctree(const FVector& Origin)
{
	TArray<FOverlapResult> Result;
	const FVector Size = FVector(SingleVolumeSize);
	const FBox Bounds = FBox(Origin, Origin + Size);

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

	OctreeNode* NewRootNode = new OctreeNode(Bounds, nullptr);
	RootNodes.Add(NewRootNode);
	NavigationGraph->AddRootNode(NewRootNode);
	//double StartTime = FPlatformTime::Seconds();
	AddObjects(Result, NewRootNode);
	//UE_LOG(LogTemp, Warning, TEXT("Generating nodes took this many seconds: %lf"), FPlatformTime::Seconds() - StartTime);
	//StartTime = FPlatformTime::Seconds();
	GetEmptyNodes(NewRootNode);
	//UE_LOG(LogTemp, Warning, TEXT("Generating navigation graph took this many seconds: %lf"), FPlatformTime::Seconds() - StartTime);
	//StartTime = FPlatformTime::Seconds();
	AdjustChildNodes(NewRootNode);
	//UE_LOG(LogTemp, Warning, TEXT("Cleaning up unnecessary nodes took this many seconds: %lf"), FPlatformTime::Seconds() - StartTime);
}

void AOctree::AddObjects(TArray<FOverlapResult> FoundObjects, OctreeNode* RootN) const
{
	if (FoundObjects.IsEmpty())
	{
		return;
	}
	
	for (auto Hit : FoundObjects)
	{
		RootN->DivideNode(Hit.GetActor(), MinNodeSize);
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
				OctreeGraphNode* NewGraphNode = new OctreeGraphNode(NodeList[i]->NodeBounds);
				NodeList[i]->GraphNode = NewGraphNode;
				NavigationGraph->AddNode(NewGraphNode);
				//DrawDebugBox(GetWorld(), NewGraphNode->Bounds.GetCenter(), NewGraphNode->Bounds.GetExtent(), FColor::Green, false, 15,0,5);
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


bool AOctree::GetAStarPath(const FVector& Start, const FVector& End, FVector& NextLocation, const AActor* Agent) const
{
	NextLocation = FVector::ZeroVector;


	TArray<FVector> OutPath;

	if (!IsSetup) return false;

	if (NavigationGraph->OctreeAStar(Start, End, OutPath))
	{
		if (FVector::Distance(Start, End) <= 20.0f)
		{
			//FGenericPlatformMisc::RequestExit(false);
			//GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Red, "Quit");
			return false;
		}

		if (OutPath.Num() == 1)
		{
			NextLocation = OutPath[0];
			return true;
		}

		//Path smoothing. If the agent can skip a path point because it wouldn't collide, it should. This ensures a more natural looking movement.
		FHitResult Hit;
		FCollisionShape ColShape = FCollisionShape::MakeSphere(Agent->GetComponentsBoundingBox().GetSize().X);
		FCollisionQueryParams TraceParams;
		TraceParams.AddIgnoredActor(Agent);

		for (int i = 2; i < OutPath.Num(); i++)
		{
			if (!GetWorld()->SweepSingleByChannel(Hit, OutPath[0], OutPath[i], FQuat::Identity, CollisionChannel, ColShape, TraceParams))
			{
				continue;
			}

			NextLocation = OutPath[i - 1];
			return true;
		}

		//If we are here then there was no path smoothing necessary.
		NextLocation = OutPath.Last();
		return true;
	}

	return false;
}

bool AOctree::GetAStarPathAsnyc(const FVector& Start, const FVector& End, FVector& NextLocation, const AActor* Agent) const
{
	if (FVector::Distance(Start, End) <= 20.0f)
	{
		//FGenericPlatformMisc::RequestExit(false);
		GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Red, "Quit");
		return false;
	}
	// Lock the mutex before accessing or modifying the flag
	std::lock_guard<std::mutex> Lock(PathfindingMutex);

	// Check if pathfinding is already in progress or if the thread is joinable
	if (IsPathfindingInProgress || (PathfindingThread && PathfindingThread->joinable()))
	{
		return false; // Pathfinding is already in progress, no need to start again
	}

	// Start the asynchronous pathfinding calculation
	PathfindingThread = std::make_unique<std::thread>([&, Start, End, Agent]()
	{
		// Lock the mutex before modifying the flag
		std::lock_guard<std::mutex> Lock(PathfindingMutex);
		IsPathfindingInProgress = true;

		// Perform pathfinding calculations here
		bool FoundPath = GetAStarPath(Start, End, NextLocation, Agent);

		IsPathfindingInProgress = false;

		return FoundPath;
	});

	// Detach the thread so it can continue running independently
	PathfindingThread->detach();

	return true; // Pathfinding started successfully
}
