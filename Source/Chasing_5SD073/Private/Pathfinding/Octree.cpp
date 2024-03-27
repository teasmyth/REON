// Fill out your copyright notice in the Description page of Project Settings.
#include "Pathfinding/Octree.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Serialization/BufferArchive.h"
#include "Pathfinding/OctreeGraph.h"


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
	ProceduralMesh->bHiddenInGame = true;
	ProceduralMesh->bUseComplexAsSimpleCollision = false;

	ProceduralMesh->bNeverDistanceCull = false;
	ProceduralMesh->SetMaterial(0, nullptr);

	static ConstructorHelpers::FObjectFinder<UMaterial> MaterialFinder(TEXT("Material'/Game/Materials/Octree/M_OctreeVisual.M_OctreeVisual'"));
	if (MaterialFinder.Object != nullptr)
	{
		OctreeMaterial = MaterialFinder.Object;
	}
}

void AOctree::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// In Tick method or wherever you want to record frame times, calculate frame time
}

void AOctree::BeginPlay()
{
	Super::BeginPlay();

	FString SpecificFileName = "OctreeFiles/" + GetWorld()->GetName() + "OctreeNodes.bin";
	SaveFileName = FPaths::Combine(FPaths::ProjectSavedDir(), SpecificFileName);

	auto LoadGame = Async(EAsyncExecution::Thread, [&]()
	{
		if (!LoadNodesFromFile(SaveFileName))
		{
			//GEngine call is not thread safe - just to note.
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, "No file found. Creating new nodes");

			SetUpOctreesAsync(false);

			SaveNodesToFile(SaveFileName);
		}

		const TWeakPtr<OctreeNode> RootNodeWeakPtr = RootNodeSharedPtr;
		PathfindingWorker = new FPathfindingWorker(RootNodeWeakPtr);
		IsSetup = true;
	});
}

#pragma region Saving/Loading Data

void AOctree::SaveNodesToFile(const FString& Filename)
{
	if (IsSetup) return; //If the game is running, don't save the nodes. Might lead to data corruption.

	FBufferArchive ToBinary;

	ToBinary << UseOverlap;
	//ToBinary << AllHitResults;
	ToBinary << RootNodeSharedPtr;


	if (FFileHelper::SaveArrayToFile(ToBinary, *Filename))
	{
		ToBinary.FlushCache();
		ToBinary.Empty();
		UE_LOG(LogTemp, Warning, TEXT("Saved octree successfully."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Couldn't save octree. Check the file path and/or permissions."));
	}
}

bool AOctree::LoadNodesFromFile(const FString& Filename)
{
	TArray<uint8> BinaryArray;

	if (FFileHelper::LoadFileToArray(BinaryArray, *Filename))
	{
		if (BinaryArray.Num() <= 0)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to deserialize octree nodes from file."));
			return false;
		}

		FMemoryReader FromBinary = FMemoryReader(BinaryArray, true); //true, free data after done
		FromBinary.Seek(0);

		bool WasUsingOverlap;
		FromBinary << WasUsingOverlap;

		if (WasUsingOverlap == UseOverlap)
		{
			SetUpOctreesAsync(true);
			//FromBinary << AllHitResults;
			FromBinary << RootNodeSharedPtr;
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, "Save File found. Loading nodes, skipping create");
			OctreeGraph::ReconstructPointersForNodes(RootNodeSharedPtr);
		}
		else
		{
			SetUpOctreesAsync(false);
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, "Save File found but incorrect. Remaking Octree");
			SaveNodesToFile(*Filename);
		}


		FromBinary.FlushCache();
		BinaryArray.Empty();
		FromBinary.Close();

		return true;
	}

	UE_LOG(LogTemp, Error, TEXT("Failed to load file into array."));
	return false;
}


#pragma endregion

void AOctree::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	IsSetup = false;
	
	// Clean up
	PathfindingWorker->Stop();
	delete PathfindingWorker;	
	DeleteOctreeNode(RootNodeSharedPtr);
}

void AOctree::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (SaveFileName == "")
	{
		SaveFileName = FPaths::Combine(FPaths::ProjectSavedDir(), "OctreeFiles/" + GetWorld()->GetName() + "OctreeNodes.bin");
	}

	if (!GridDrawn)
	{
		ResizeOctree();
		CalculateBorders();
	}
	else
	{
		DrawGrid();
	}
}

#pragma region Drawing
void AOctree::DrawGrid()
{
	if (GridDrawn)
	{
		DynamicMaterialInstance->SetVectorParameterValue("Color", Color);
		DynamicMaterialInstance->SetScalarParameterValue("Opacity", Color.A);
		return;
	}


	if (RootNodeSharedPtr == nullptr)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, "Root node is null. Cannot draw grid.");
		}
	}

	GridDrawn = true;
	ProceduralMesh->ClearAllMeshSections();
	ProceduralMesh->bHiddenInGame = false;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;

	TArray<TSharedPtr<OctreeNode>> NodeList;
	NodeList.Add(RootNodeSharedPtr);

	for (int i = 0; i < NodeList.Num(); i++)
	{
		for (const auto& Child : NodeList[i]->ChildrenOctreeNodes)
		{
			if (Child->NavigationNode)
			{
				const FVector Center = Child->NodeBounds.GetCenter();
				const FVector Extent = Child->NodeBounds.GetExtent();

				// Draw lines for each edge of the cube
				DrawLine(Center - Extent, FVector(Center.X + Extent.X, Center.Y - Extent.Y, Center.Z - Extent.Z), FVector::RightVector, Vertices,
				         Triangles);
				DrawLine(FVector(Center.X + Extent.X, Center.Y - Extent.Y, Center.Z - Extent.Z),
				         FVector(Center.X + Extent.X, Center.Y - Extent.Y, Center.Z + Extent.Z), FVector::ForwardVector, Vertices, Triangles);
				DrawLine(FVector(Center.X + Extent.X, Center.Y - Extent.Y, Center.Z + Extent.Z),
				         FVector(Center.X - Extent.X, Center.Y - Extent.Y, Center.Z + Extent.Z), FVector::RightVector, Vertices, Triangles);
				DrawLine(FVector(Center.X - Extent.X, Center.Y - Extent.Y, Center.Z + Extent.Z),
				         FVector(Center.X - Extent.X, Center.Y - Extent.Y, Center.Z - Extent.Z), FVector::ForwardVector, Vertices, Triangles);

				DrawLine(FVector(Center.X - Extent.X, Center.Y + Extent.Y, Center.Z - Extent.Z),
				         FVector(Center.X + Extent.X, Center.Y + Extent.Y, Center.Z - Extent.Z), FVector::RightVector, Vertices, Triangles);
				DrawLine(FVector(Center.X + Extent.X, Center.Y + Extent.Y, Center.Z - Extent.Z),
				         FVector(Center.X + Extent.X, Center.Y + Extent.Y, Center.Z + Extent.Z), FVector::ForwardVector, Vertices, Triangles);
				DrawLine(FVector(Center.X + Extent.X, Center.Y + Extent.Y, Center.Z + Extent.Z),
				         FVector(Center.X - Extent.X, Center.Y + Extent.Y, Center.Z + Extent.Z), FVector::RightVector, Vertices, Triangles);
				DrawLine(FVector(Center.X - Extent.X, Center.Y + Extent.Y, Center.Z + Extent.Z),
				         FVector(Center.X - Extent.X, Center.Y + Extent.Y, Center.Z - Extent.Z), FVector::ForwardVector, Vertices, Triangles);

				DrawLine(FVector(Center.X - Extent.X, Center.Y - Extent.Y, Center.Z - Extent.Z),
				         FVector(Center.X - Extent.X, Center.Y + Extent.Y, Center.Z - Extent.Z), FVector::UpVector, Vertices, Triangles);
				DrawLine(FVector(Center.X - Extent.X, Center.Y - Extent.Y, Center.Z + Extent.Z),
				         FVector(Center.X - Extent.X, Center.Y + Extent.Y, Center.Z + Extent.Z), FVector::UpVector, Vertices, Triangles);
				DrawLine(FVector(Center.X + Extent.X, Center.Y - Extent.Y, Center.Z - Extent.Z),
				         FVector(Center.X + Extent.X, Center.Y + Extent.Y, Center.Z - Extent.Z), FVector::UpVector, Vertices, Triangles);
				DrawLine(FVector(Center.X + Extent.X, Center.Y + Extent.Y, Center.Z + Extent.Z),
				         FVector(Center.X + Extent.X, Center.Y + Extent.Y, Center.Z - Extent.Z), FVector::UpVector, Vertices, Triangles);
			}
			NodeList.Add(Child);
		}
	}

	// Unused variables that are required to be passed to CreateMeshSection
	const TArray<FVector> Normals;
	const TArray<FVector2D> UVs;
	const TArray<FColor> Colors;
	const TArray<FProcMeshTangent> Tangents;

	// Add the geometry to the procedural mesh so it will render
	ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);

	if (DynamicMaterialInstance == nullptr)
	{
		DynamicMaterialInstance = UMaterialInstanceDynamic::Create(OctreeMaterial, this);
	}

	DynamicMaterialInstance->SetVectorParameterValue("Color", Color);
	DynamicMaterialInstance->SetScalarParameterValue("Opacity", Color.A);
	ProceduralMesh->SetMaterial(0, DynamicMaterialInstance);
}

void AOctree::DeleteGrid()
{
	GridDrawn = false;
	CalculateBorders();
}

void AOctree::CalculateBorders()
{
	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	TArray<FVector> OctreeVertices;
	TArray<int32> OctreeTriangles;
	ProceduralMesh->bHiddenInGame = true;
	ProceduralMesh->ClearAllMeshSections();

	for (int32 y = 0; y <= GetOctreeSizeY(); y += GetOctreeSizeY())
	{
		for (int32 z = 0; z <= GetOctreeSizeZ(); z += GetOctreeSizeZ())
		{
			Start = FVector(0, y, z) + GetActorLocation();
			End = FVector(GetOctreeSizeX(), y, z) + GetActorLocation();
			DrawLine(Start, End, FVector::UpVector, OctreeVertices, OctreeTriangles);
		}
	}

	for (int32 x = 0; x <= GetOctreeSizeX(); x += GetOctreeSizeX())
	{
		for (int32 z = 0; z <= GetOctreeSizeZ(); z += GetOctreeSizeZ())
		{
			Start = FVector(x, 0, z) + GetActorLocation();
			End = FVector(x, GetOctreeSizeY(), z) + GetActorLocation();
			DrawLine(Start, End, FVector::UpVector, OctreeVertices, OctreeTriangles);
		}
	}

	for (int32 x = 0; x <= GetOctreeSizeX(); x += GetOctreeSizeX())
	{
		for (int32 y = 0; y <= GetOctreeSizeY(); y += GetOctreeSizeY())
		{
			Start = FVector(x, y, 0) + GetActorLocation();
			End = FVector(x, y, GetOctreeSizeZ()) + GetActorLocation();
			DrawLine(Start, End, FVector::ForwardVector, OctreeVertices, OctreeTriangles);
		}
	}

	const TArray<FVector> Normals;
	const TArray<FVector2D> UVs;
	const TArray<FColor> Colors;
	const TArray<FProcMeshTangent> Tangents;

	ProceduralMesh->CreateMeshSection(0, OctreeVertices, OctreeTriangles, Normals, UVs, Colors, Tangents, false);

	if (DynamicMaterialInstance == nullptr)
	{
		DynamicMaterialInstance = UMaterialInstanceDynamic::Create(OctreeMaterial, this);
	}

	DynamicMaterialInstance->SetVectorParameterValue("Color", Color);
	DynamicMaterialInstance->SetScalarParameterValue("Opacity", Color.A);
	ProceduralMesh->SetMaterial(0, DynamicMaterialInstance);
}

void AOctree::DrawLine(const FVector& Start, const FVector& End, const FVector& Normal, TArray<FVector>& Vertices, TArray<int32>& Triangles) const
{
	// Calculate the half line thickness and the thickness direction
	const float HalfLineThickness = GridLineThickness / 2.0f;

	const FVector NewStart = Start - GetActorLocation();
	const FVector NewEnd = End - GetActorLocation();
	FVector Line = NewEnd - NewStart;
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
		Vertices.Add(NewStart + (ThicknessDirection * HalfLineThickness));
		// Vertex 1
		Vertices.Add(NewEnd + (ThicknessDirection * HalfLineThickness));
		// Vertex 2
		Vertices.Add(NewStart - (ThicknessDirection * HalfLineThickness));
		// Vertex 3
		Vertices.Add(NewEnd - (ThicknessDirection * HalfLineThickness));
	};

	const FVector Direction1 = UKismetMathLibrary::Cross_VectorVector(Line, Normal);
	const FVector Direction2 = UKismetMathLibrary::Cross_VectorVector(Line, Direction1);

	CreateFlatLine(Direction1);
	CreateFlatLine(Direction2);
}


#pragma endregion

void AOctree::DeleteOctreeNode(TSharedPtr<OctreeNode>& Node)
{
	// Traverse the octree from the root node to the leaf nodes
	for (TSharedPtr<OctreeNode>& Child : Node->ChildrenOctreeNodes)
	{
		if (Child != nullptr)
		{
			// Recursively delete child nodes
			DeleteOctreeNode(Child);
		}
	}

	// Set the node's TSharedPtr to nullptr
	Node = nullptr;
}

void AOctree::BakeOctree()
{
	if (!IsSetup)
	{
		UE_LOG(LogTemp, Warning, TEXT("Baking Octree"));
		SetUpOctreesAsync(false);
		SaveNodesToFile(SaveFileName);
		//delete RootNode;
	}
	else UE_LOG(LogTemp, Warning, TEXT("Game is running, cant bake Octree."));
}

#pragma region Making Octree

void AOctree::ResizeOctree()
{
	if (AutoEncapsulateObjects)
	{
		// Choose the appropriate base class based on your requirements
		UClass* ActorBaseClass = AActor::StaticClass();

		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ActorBaseClass, Actors);

		FBox EnclosingBox = FBox();

		for (const AActor* Actor : Actors)
		{
			if (!ActorToIgnore.Contains(Actor) && Actor->GetRootComponent() && Actor->GetRootComponent()->GetCollisionObjectType() ==
				CollisionChannel && Actor->FindComponentByClass<UStaticMeshComponent>())
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
}

void AOctree::SetUpOctreesAsync(const bool IsLoading)
{
	FVector Size = FVector(ExpandVolumeXAxis * SingleVolumeSize, ExpandVolumeYAxis * SingleVolumeSize, ExpandVolumeZAxis * SingleVolumeSize);
	//Add a little bit of padding, in case there is one single Octree underneath, which sometimes prevent FindNode to work properly.
	Size *= 1.02f;
	//RootNode = new OctreeNode(FBox(GetActorLocation(), GetActorLocation() + Size), nullptr);
	//RootNodeSharedPtr = MakeShareable(RootNode);

	RootNodeSharedPtr = MakeShareable(new OctreeNode(FBox(GetActorLocation(), GetActorLocation() + Size), nullptr, false));

	RootNodeSharedPtr->ChildrenOctreeNodes.SetNum(ExpandVolumeXAxis * ExpandVolumeYAxis * ExpandVolumeZAxis);
	/*
	if (!AllHitResults.IsEmpty() && AllHitResults.Num() != ExpandVolumeXAxis * ExpandVolumeYAxis * ExpandVolumeZAxis)
	{
		//Meaning our previous results are completely outdated.
		AllHitResults.Empty();
	}
	*/

	if (IsLoading) return;

	int Index = 0;
	for (int X = 0; X < ExpandVolumeXAxis; X++)
	{
		for (int Y = 0; Y < ExpandVolumeYAxis; Y++)
		{
			for (int Z = 0; Z < ExpandVolumeZAxis; Z++)
			{
				const FVector Offset = FVector(X * SingleVolumeSize, Y * SingleVolumeSize, Z * SingleVolumeSize);
				RootNodeSharedPtr->ChildrenOctreeNodes[Index] = MakeOctree(GetActorLocation() + Offset, Index);
				Index++;
			}
		}
	}
	GetEmptyNodes(RootNodeSharedPtr);
	AdjustDanglingChildNodes(RootNodeSharedPtr);
	OctreeGraph::ConnectNodes(RootNodeSharedPtr);
}

TSharedPtr<OctreeNode> AOctree::MakeOctree(const FVector& Origin, const int& Index)
{
	const FVector Size = FVector(SingleVolumeSize);
	const FBox Bounds = FBox(Origin, Origin + Size);
	TSharedPtr<OctreeNode> NewRootNode = MakeShareable(new OctreeNode(Bounds, nullptr, false));

	if (!UseOverlap)
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


		TArray<FBox> BoxResults;
		for (const auto Overlap : Result)
		{
			BoxResults.Add(Overlap.GetActor()->GetComponentsBoundingBox());
		}

		AddObjects(BoxResults, NewRootNode);

		/*
		if (!AllHitResults.IsEmpty())
		{
			TArray<FBox> NewResults;
			for (auto OverlapBox : AllHitResults[Index])
			{
				if (!BoxResults.Contains(OverlapBox))
				{
					NewResults.Add(OverlapBox);
				}
			}


			for (auto OverlapBox : BoxResults)
			{
				if (!AllHitResults[Index].Contains(OverlapBox))
				{
					NewResults.Add(OverlapBox);
				}
			}


			for (auto NewResult : NewResults)
			{
				UE_LOG(LogTemp, Warning, TEXT("New result: %f, %f, %f"), NewResult.GetCenter().X, NewResult.GetCenter().Y, NewResult.GetCenter().Z);
			}

			AddObjects(NewResults, NewRootNode);
			
		}
		else
		{
			AddObjects(BoxResults, NewRootNode);
		}
		AllHitResults.Add(BoxResults);
		*/
	}
	else
	{
		NewRootNode->DivideNode(FBox(), MinNodeSize, FloatAboveGroundPreference, GetWorld(), false);
	}


	return NewRootNode;
}


void AOctree::AddObjects(TArray<FBox> FoundObjects, const TSharedPtr<OctreeNode>& RootN) const
{
	if (FoundObjects.IsEmpty())
	{
		return;
	}

	for (const auto& Box : FoundObjects)
	{
		RootN->DivideNode(Box, MinNodeSize, FloatAboveGroundPreference, GetWorld(), true);
	}
}

void AOctree::GetEmptyNodes(const TSharedPtr<OctreeNode>& Node)
{
	if (Node == nullptr)
	{
		return;
	}

	TArray<TSharedPtr<OctreeNode>> NodeList;
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
				NodeList[i]->NavigationNode = true;
			}
			else
			{
				const int Index = NodeList[i]->Parent->ChildrenOctreeNodes.Find(NodeList[i]);
				NodeList[i]->Parent->ChildrenOctreeNodes[Index] = nullptr;
				//delete NodeList[i];
				//I adjust the null pointers in AdjustDanglingChildNodes(). Here I should not touch it while it is iterating through it.
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

void AOctree::AdjustDanglingChildNodes(const TSharedPtr<OctreeNode>& Node)
{
	if (Node == nullptr)
	{
		return;
	}

	TArray<TSharedPtr<OctreeNode>> NodeList;
	NodeList.Add(Node);

	for (int i = 0; i < NodeList.Num(); i++)
	{
		const TSharedPtr<OctreeNode> CurrentNode = NodeList[i];
		TArray<TSharedPtr<OctreeNode>> CleanedChildNodes;
		for (auto Child : CurrentNode->ChildrenOctreeNodes)
		{
			if (Child != nullptr)
			{
				CleanedChildNodes.Add(Child);
			}
		}

		CurrentNode->ChildrenOctreeNodes = CleanedChildNodes;

		for (const auto& Child : CurrentNode->ChildrenOctreeNodes)
		{
			NodeList.Add(Child);
		}
	}
}

#pragma endregion

#pragma region Pathfinding

#pragma endregion
