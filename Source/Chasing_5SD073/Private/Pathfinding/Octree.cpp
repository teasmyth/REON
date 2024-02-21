// Fill out your copyright notice in the Description page of Project Settings.
#include "Pathfinding/Octree.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Serialization/BufferArchive.h"


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

	ProceduralMesh->bNeverDistanceCull = false;
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

	UE_LOG(LogTemp, Warning, TEXT("Octree Begin Play"));
	
	NavigationGraph =  OctreeGraph();
	PreviousNextLocation = FVector::ZeroVector;
	FString SpecificFileName = "OctreeFiles/" + GetWorld()->GetName() + "OctreeNodes.bin";
	SaveFileName = FPaths::Combine(FPaths::ProjectSavedDir(), SpecificFileName);

	//UE_LOG(LogTemp, Warning, TEXT("Saved dir: %s"), *FPaths::ProjectSavedDir());

	SetUpOctreesAsync(false);
	IsSetup = true;

	/*
	if (!LoadNodesFromFile(SaveFileName))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, "No file found. Creating new nodes");

		SetUpOctreesAsync(false);

		SaveNodesToFile(SaveFileName);
		IsSetup = true;

		//auto TurnTimeToNormalAsync = Async(EAsyncExecution::Thread, [&]()
		//{
		// if (ShowGridAfterCalculation)
		// {
		// 	ShowGrid();
		// }

		//});
	}
	else
	{
		IsSetup = true;
	}
	*/
}

#pragma region Saving/Loading Data

void AOctree::SaveNodesToFile(const FString& Filename)
{
	if (IsSetup) return; //If the game is running, don't save the nodes. Might lead to data corruption.

	FBufferArchive ToBinary;

	ToBinary << AllHitResults;
	ToBinary << RootNode;

	//ToBinary << NavigationGraph->NodeBounds;

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
		SetUpOctreesAsync(true);

		//Todo: define how to save
		FromBinary << AllHitResults;
		FromBinary << RootNode;

		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, "Save File found. Loading nodes, skipping create");
		FromBinary.FlushCache();
		BinaryArray.Empty();
		FromBinary.Close();


		NavigationGraph.ReconstructPointersForNodes(RootNode);
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
	SaveNodesToFile(SaveFileName);

	delete RootNode;
	//delete NavigationGraph;
}

void AOctree::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ResizeOctree();
	DrawOctree();
}

#pragma region Drawing
void AOctree::DrawGrid() const
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, "No mesh data found. Creating borders instead");

	ProceduralMesh->ClearAllMeshSections();

	// Create arrays to store the vertices and the triangles
	TArray<FVector> Vertices;
	TArray<int32> Triangles;

	// Define variables for the start and end of the line
	TSet<TPair<FVector, FVector>> CreatedLines;

	TArray<OctreeNode*> NodeList;
	NodeList.Add(RootNode);

	for (int i = 0; i < NodeList.Num(); i++)
	{
		for (const auto& Child : NodeList[i]->ChildrenOctreeNodes)
		{
			if (Child->NavigationNode)
			{
				TPair<FVector, FVector> Pairs[12] =
				{
					TPair<FVector, FVector>(Child->NodeBounds.Min,
					                        FVector(Child->NodeBounds.Max.X, Child->NodeBounds.Min.Y, Child->NodeBounds.Min.Z)),
					TPair<FVector, FVector>(FVector(Child->NodeBounds.Max.X, Child->NodeBounds.Min.Y, Child->NodeBounds.Min.Z),
					                        FVector(Child->NodeBounds.Max.X, Child->NodeBounds.Min.Y, Child->NodeBounds.Max.Z)),
					TPair<FVector, FVector>(FVector(Child->NodeBounds.Max.X, Child->NodeBounds.Min.Y, Child->NodeBounds.Max.Z),
					                        FVector(Child->NodeBounds.Min.X, Child->NodeBounds.Min.Y, Child->NodeBounds.Max.Z)),
					TPair<FVector, FVector>(FVector(Child->NodeBounds.Min.X, Child->NodeBounds.Min.Y, Child->NodeBounds.Max.Z),
					                        Child->NodeBounds.Min),
					TPair<FVector, FVector>(FVector(Child->NodeBounds.Min.X, Child->NodeBounds.Max.Y, Child->NodeBounds.Min.Z),
					                        FVector(Child->NodeBounds.Max.X, Child->NodeBounds.Max.Y, Child->NodeBounds.Min.Z)),
					TPair<FVector, FVector>(FVector(Child->NodeBounds.Max.X, Child->NodeBounds.Max.Y, Child->NodeBounds.Min.Z),
					                        Child->NodeBounds.Max),
					TPair<FVector, FVector>(Child->NodeBounds.Max,
					                        FVector(Child->NodeBounds.Min.X, Child->NodeBounds.Max.Y, Child->NodeBounds.Max.Z)),
					TPair<FVector, FVector>(FVector(Child->NodeBounds.Min.X, Child->NodeBounds.Max.Y, Child->NodeBounds.Max.Z),
					                        FVector(Child->NodeBounds.Min.X, Child->NodeBounds.Max.Y, Child->NodeBounds.Min.Z)),
					TPair<FVector, FVector>(Child->NodeBounds.Min,
					                        FVector(Child->NodeBounds.Min.X, Child->NodeBounds.Max.Y, Child->NodeBounds.Min.Z)),
					TPair<FVector, FVector>(FVector(Child->NodeBounds.Min.X, Child->NodeBounds.Min.Y, Child->NodeBounds.Max.Z),
					                        FVector(Child->NodeBounds.Min.X, Child->NodeBounds.Max.Y, Child->NodeBounds.Max.Z)),
					TPair<FVector, FVector>(FVector(Child->NodeBounds.Max.X, Child->NodeBounds.Min.Y, Child->NodeBounds.Min.Z),
					                        FVector(Child->NodeBounds.Max.X, Child->NodeBounds.Max.Y, Child->NodeBounds.Min.Z)),
					TPair<FVector, FVector>(FVector(Child->NodeBounds.Max.X, Child->NodeBounds.Max.Y, Child->NodeBounds.Min.Z), Child->NodeBounds.Max)
				};

				for (const auto& Pair : Pairs)
				{
					if (!CreatedLines.Contains(Pair))
					{
						FVector NormalVector = (Pair.Value - Pair.Key).GetSafeNormal();
						if (NormalVector.X > 0)
						{
							NormalVector = FVector::RightVector;
						}
						DrawLine(Pair.Key, Pair.Value, NormalVector, Vertices, Triangles);
						CreatedLines.Add(Pair);
					}
				}
				/*
				DrawLine(Child->NodeBounds.Min, FVector(Child->NodeBounds.Max.X, Child->NodeBounds.Min.Y, Child->NodeBounds.Min.Z), FVector::RightVector, Vertices, Triangles, GridLineThickness);
				DrawLine(FVector(Child->NodeBounds.Max.X, Child->NodeBounds.Min.Y, Child->NodeBounds.Min.Z), FVector(Child->NodeBounds.Max.X, Child->NodeBounds.Min.Y, Child->NodeBounds.Max.Z), FVector::ForwardVector, Vertices, Triangles, GridLineThickness);
				DrawLine(FVector(Child->NodeBounds.Max.X, Child->NodeBounds.Min.Y, Child->NodeBounds.Max.Z), FVector(Child->NodeBounds.Min.X, Child->NodeBounds.Min.Y, Child->NodeBounds.Max.Z), FVector::UpVector, Vertices, Triangles, GridLineThickness);
				DrawLine(FVector(Child->NodeBounds.Min.X, Child->NodeBounds.Min.Y, Child->NodeBounds.Max.Z), Child->NodeBounds.Min, FVector::ForwardVector, Vertices, Triangles, GridLineThickness);
				DrawLine(FVector(Child->NodeBounds.Min.X, Child->NodeBounds.Max.Y, Child->NodeBounds.Min.Z), FVector(Child->NodeBounds.Max.X, Child->NodeBounds.Max.Y, Child->NodeBounds.Min.Z), FVector::RightVector, Vertices, Triangles, GridLineThickness);
				DrawLine(FVector(Child->NodeBounds.Max.X, Child->NodeBounds.Max.Y, Child->NodeBounds.Min.Z), Child->NodeBounds.Max, FVector::ForwardVector, Vertices, Triangles, GridLineThickness);
				DrawLine(Child->NodeBounds.Max, FVector(Child->NodeBounds.Min.X, Child->NodeBounds.Max.Y, Child->NodeBounds.Max.Z), FVector::RightVector, Vertices, Triangles, GridLineThickness);
				DrawLine(FVector(Child->NodeBounds.Min.X, Child->NodeBounds.Max.Y, Child->NodeBounds.Max.Z), FVector(Child->NodeBounds.Min.X, Child->NodeBounds.Max.Y, Child->NodeBounds.Min.Z), FVector::ForwardVector, Vertices, Triangles, GridLineThickness);
				DrawLine(Child->NodeBounds.Min, FVector(Child->NodeBounds.Min.X, Child->NodeBounds.Max.Y, Child->NodeBounds.Min.Z), FVector::UpVector, Vertices, Triangles, GridLineThickness);
				DrawLine(FVector(Child->NodeBounds.Min.X, Child->NodeBounds.Min.Y, Child->NodeBounds.Max.Z), FVector(Child->NodeBounds.Min.X, Child->NodeBounds.Max.Y, Child->NodeBounds.Max.Z), FVector::UpVector, Vertices, Triangles, GridLineThickness);
				DrawLine(FVector(Child->NodeBounds.Max.X, Child->NodeBounds.Min.Y, Child->NodeBounds.Min.Z), FVector(Child->NodeBounds.Max.X, Child->NodeBounds.Max.Y, Child->NodeBounds.Min.Z), FVector::UpVector, Vertices, Triangles, GridLineThickness);
				DrawLine(FVector(Child->NodeBounds.Max.X, Child->NodeBounds.Max.Y, Child->NodeBounds.Min.Z), Child->NodeBounds.Max, FVector::UpVector, Vertices, Triangles, GridLineThickness);
*/
			}
			NodeList.Add(Child);
		}
	}


	// Unused variables that are required to be passed to CreateMeshSection
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	// Add the geometry to the procedural mesh so it will render
	ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);


	// Set the material on the procedural mesh so it's color/opacity can be configurable
	DynamicMaterialInstance->SetVectorParameterValue("Color", Color);
	//DynamicMaterialInstance->SetScalarParameterValue("Opacity", Color.A);
	ProceduralMesh->SetMaterial(0, DynamicMaterialInstance);
}

void AOctree::DeleteGrid() const
{
	
}

void AOctree::CalculateBorders() 
{
	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	TArray<FVector> OctreeVertices;
	TArray<int32> OctreeTriangles;

	for (int32 y = 0; y <= GetOctreeSizeY(); y += GetOctreeSizeY())
	{
		for (int32 z = 0; z <= GetOctreeSizeZ(); z += GetOctreeSizeZ())
		{
			Start = FVector(0, y, z);
			End = FVector(GetOctreeSizeX(), y, z);
			DrawLine(Start, End, FVector::UpVector, OctreeVertices, OctreeTriangles);
		}
	}

	for (int32 x = 0; x <= GetOctreeSizeX(); x += GetOctreeSizeX())
	{
		for (int32 z = 0; z <= GetOctreeSizeZ(); z += GetOctreeSizeZ())
		{
			Start = FVector(x, 0, z);
			End = FVector(x, GetOctreeSizeY(), z);
			DrawLine(Start, End, FVector::UpVector, OctreeVertices, OctreeTriangles);
		}
	}

	for (int32 x = 0; x <= GetOctreeSizeX(); x += GetOctreeSizeX())
	{
		for (int32 y = 0; y <= GetOctreeSizeY(); y += GetOctreeSizeY())
		{
			Start = FVector(x, y, 0);
			End = FVector(x, y, GetOctreeSizeZ());
			DrawLine(Start, End, FVector::ForwardVector, OctreeVertices, OctreeTriangles);
		}
	}

	/*
	// Unused variables that are required to be passed to CreateMeshSection
	const TArray<FVector> Normals;
	const TArray<FVector2D> UVs;
	const TArray<FColor> Colors;
	const TArray<FProcMeshTangent> Tangents;

	// Add the geometry to the procedural mesh so it will render
	ProceduralMesh->CreateMeshSection(0, OctreeVertices, OctreeTriangles, Normals, UVs, Colors, Tangents, false);

	// Set the material on the procedural mesh so it's color/opacity can be configurable
	//UMaterialInstanceDynamic* DynamicMaterialInstance = UMaterialInstanceDynamic::Create(OctreeMaterial, this);
	DynamicMaterialInstance->SetVectorParameterValue("Color", Color);
	//DynamicMaterialInstance->SetScalarParameterValue("Opacity", 0.5f);
	//ProceduralMesh->SetMaterial(0, DynamicMaterialInstance);
	*/

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
	ProceduralMesh->SetMaterial(0, DynamicMaterialInstance);
}

void AOctree::DrawLine(const FVector& Start, const FVector& End, const FVector& Normal, TArray<FVector>& Vertices, TArray<int32>& Triangles) const
{
	// Calculate the half line thickness and the thickness direction
	const float HalfLineThickness = GridLineThickness / 2.0f;
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

void AOctree::DrawOctree()
{
	//Defining the colliders borders

	if (OctreeMaterial == nullptr || GridDrawn) return;

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

	TArray<FVector> OctreeVertices;
	TArray<int32> OctreeTriangles;

	ProceduralMesh->ClearAllMeshSections();
	CalculateBorders();

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
	ProceduralMesh->SetMaterial(0, DynamicMaterialInstance);
}

#pragma endregion

void AOctree::BakeOctree()
{
	UE_LOG(LogTemp, Warning, TEXT("Baking octree"));
	SetUpOctreesAsync(false);
	DrawOctree();
	SaveNodesToFile(SaveFileName);
	delete RootNode;
	//delete NavigationGraph;
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
}

void AOctree::SetUpOctreesAsync(const bool IsLoading)
{
	FVector Size = FVector(ExpandVolumeXAxis * SingleVolumeSize, ExpandVolumeYAxis * SingleVolumeSize, ExpandVolumeZAxis * SingleVolumeSize);
	//Add a little bit of padding, in case there is one single Octree underneath, which would prevent FindNode to work properly.
	//As they would have the same FBox.
	Size *= 1.02f;
	RootNode = new OctreeNode(FBox(GetActorLocation(), GetActorLocation() + Size), nullptr);

	RootNode->ChildrenOctreeNodes.SetNum(ExpandVolumeXAxis * ExpandVolumeYAxis * ExpandVolumeZAxis);
	if (!AllHitResults.IsEmpty() && AllHitResults.Num() != ExpandVolumeXAxis * ExpandVolumeYAxis * ExpandVolumeZAxis)
	{
		//Meaning our previous results are completely outdated.
		AllHitResults.Empty();
	}

	if (IsLoading) return;

	int Index = 0;
	for (int X = 0; X < ExpandVolumeXAxis; X++)
	{
		for (int Y = 0; Y < ExpandVolumeYAxis; Y++)
		{
			for (int Z = 0; Z < ExpandVolumeZAxis; Z++)
			{
				const FVector Offset = FVector(X * SingleVolumeSize, Y * SingleVolumeSize, Z * SingleVolumeSize);
				RootNode->ChildrenOctreeNodes[Index] = MakeOctree(GetActorLocation() + Offset, Index);
				Index++;
			}
		}
	}
	GetEmptyNodes(RootNode);
	AdjustDanglingChildNodes(RootNode);
	NavigationGraph.ConnectNodes(RootNode);
	UE_LOG(LogTemp, Warning, TEXT("Octree setup"));
}

OctreeNode* AOctree::MakeOctree(const FVector& Origin, const int& Index)
{
	const FVector Size = FVector(SingleVolumeSize);
	const FBox Bounds = FBox(Origin, Origin + Size);
	OctreeNode* NewRootNode = new OctreeNode(Bounds, nullptr);

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


		TArray<FBox> BoxResults;
		for (const auto Overlap : Result)
		{
			BoxResults.Add(Overlap.GetActor()->GetComponentsBoundingBox());
		}

		
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

			/*
			for (auto NewResult : NewResults)
			{
				UE_LOG(LogTemp, Warning, TEXT("New result: %s"), *NewResult.GetCenter());
			}
			*/

			AddObjects(NewResults, NewRootNode);
		}
		else
		{
			AddObjects(BoxResults, NewRootNode);
		}
		AllHitResults.Add(BoxResults);
	}
	else
	{
		NewRootNode->DivideNode(FBox(), MinNodeSize, GetWorld(), false);
	}

	
	return NewRootNode;
}


void AOctree::AddObjects(TArray<FBox> FoundObjects, OctreeNode* RootN) const
{
	if (FoundObjects.IsEmpty())
	{
		return;
	}

	for (const auto& Box : FoundObjects)
	{
		RootN->DivideNode(Box, MinNodeSize, GetWorld(), true);
	}
}

void AOctree::GetEmptyNodes(OctreeNode* Node)
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

		NodeList[i]->ID = NodeID;
		NodeID++;

		if (NodeList[i]->ChildrenOctreeNodes.IsEmpty())
		{
			if (!NodeList[i]->Occupied) //NodeList[i]->ContainedActors.IsEmpty())
			{
				NodeList[i]->NavigationNode = true;
				
				//DrawDebugBox(GetWorld(), NodeList[i]->NodeBounds.GetCenter(), NodeList[i]->NodeBounds.GetExtent(), FColor::Green, true, 10.0f);
			}
			else
			{
				
				const int Index = NodeList[i]->Parent->ChildrenOctreeNodes.Find(NodeList[i]);
				NodeList[i]->Parent->ChildrenOctreeNodes[Index] = nullptr;
				delete NodeList[i];
				
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

void AOctree::AdjustDanglingChildNodes(OctreeNode* Node)
{
	if (Node == nullptr)
	{
		return;
	}

	TArray<OctreeNode*> NodeList;
	NodeList.Add(Node);

	for (int i = 0; i < NodeList.Num(); i++)
	{
		OctreeNode* CurrentNode = NodeList[i];
		TArray<OctreeNode*> CleanedChildNodes;
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

bool AOctree::GetAStarPath(const AActor* Agent, const FVector& End, FVector& OutNextLocation)
{
	if (!IsSetup)
	{
		return false;
	}

	const FVector Start = Agent->GetActorLocation();

	if (TArray<FVector> OutPath; NavigationGraph.OctreeAStar(Start, End, RootNode, OutPath))
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

#pragma endregion
