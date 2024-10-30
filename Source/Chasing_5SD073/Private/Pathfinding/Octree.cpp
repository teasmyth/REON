// Fill out your copyright notice in the Description page of Project Settings.
#include "Pathfinding/Octree.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Pathfinding/OctreePathfindingComponent.h"


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
	if (RootNodeSharedPtr.IsValid())
	{
		//DrawGrid();
	}
}

void AOctree::BeginPlay()
{
	Super::BeginPlay();


	SetUpOctree();
	IsSetup = true;
}

#pragma endregion

void AOctree::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	IsSetup = false;
	Loading = false;

	// Clean up
	if (PathfindingWorker.IsValid())
	{
		PathfindingWorker->Stop();
		PathfindingWorker->Exit();
		PathfindingWorker.Reset();
	}

	OctreeNode::DeleteOctreeNode(RootNodeSharedPtr);
}

void AOctree::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

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
	/*
	TArray<TSharedPtr<OctreeNode>> NodeList;
	NodeList.Add(RootNodeSharedPtr);
	

	for (int i = 0; i < NodeList.Num(); i++)
	{
		for (const auto& Child : NodeList[i]->ChildrenOctreeNodes)
		{
			if (Child.IsValid())
			{
				if (!Child->Occupied && Child->ChildrenOctreeNodes.Num() == 0)
				{
					DrawDebugBox(GetWorld(), Child->Position, FVector(Child->HalfSize), FColor::White, false, 0, 0, 10);
				}

				for (const auto& GrandChild : Child->ChildrenOctreeNodes)
				{
					if (GrandChild.IsValid())
					{
						NodeList.Add(GrandChild);
					}
				}
			}
		}
	}
	*/

	/*
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
	*/

	//GridDrawn = true;
	ProceduralMesh->ClearAllMeshSections();
	ProceduralMesh->bHiddenInGame = false;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;

	TArray<TSharedPtr<OctreeNode>> NodeList;
	NodeList.Add(RootNodeSharedPtr);


	for (int i = 0; i < NodeList.Num(); i++)
	{
		if (!NodeList[i].IsValid()) continue;

		for (const auto& Child : NodeList[i]->ChildrenOctreeNodes)
		{
			if (Child.IsValid())
			{
				if (!Child->Occupied && Child->ChildrenOctreeNodes.Num() == 0)
				{
					const FVector Center = Child->Position;
					const FVector Extent = FVector(Child->HalfSize);

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

	for (int32 y = -GetOctreeSizeY() / 2.0f; y <= GetOctreeSizeY() / 2.0f; y += GetOctreeSizeY())
	{
		for (int32 z = -GetOctreeSizeZ() / 2.0f; z <= GetOctreeSizeZ() / 2.0f; z += GetOctreeSizeZ())
		{
			Start = FVector(-GetOctreeSizeX() / 2.0f, y, z) + GetActorLocation();
			End = FVector(GetOctreeSizeX() / 2.0f, y, z) + GetActorLocation();
			DrawLine(Start, End, FVector::UpVector, OctreeVertices, OctreeTriangles);
		}
	}

	for (int32 x = -GetOctreeSizeX() / 2.0f; x <= GetOctreeSizeX() / 2.0f; x += GetOctreeSizeX())
	{
		for (int32 z = -GetOctreeSizeZ() / 2.0f; z <= GetOctreeSizeZ() / 2.0f; z += GetOctreeSizeZ())
		{
			Start = FVector(x, -GetOctreeSizeY() / 2.0f, z) + GetActorLocation();
			End = FVector(x, GetOctreeSizeY() / 2.0f, z) + GetActorLocation();
			DrawLine(Start, End, FVector::UpVector, OctreeVertices, OctreeTriangles);
		}
	}

	for (int32 x = -GetOctreeSizeX() / 2.0f; x <= GetOctreeSizeX() / 2.0f; x += GetOctreeSizeX())
	{
		for (int32 y = -GetOctreeSizeY() / 2.0f; y <= GetOctreeSizeY() / 2.0f; y += GetOctreeSizeY())
		{
			Start = FVector(x, y, -GetOctreeSizeZ() / 2.0f) + GetActorLocation();
			End = FVector(x, y, GetOctreeSizeZ() / 2.0f) + GetActorLocation();
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
			if (!ActorsToIgnore.Contains(Actor) && Actor->GetRootComponent() && Actor->GetRootComponent()->GetCollisionObjectType() ==
				CollisionChannel && Actor->FindComponentByClass<UStaticMeshComponent>())
			{
				EnclosingBox += Actor->GetComponentsBoundingBox();
			}
		}

		SingleVolumeSize = FMath::Max3(EnclosingBox.GetSize().X, EnclosingBox.GetSize().Y, EnclosingBox.GetSize().Z);
		SetActorLocation(EnclosingBox.GetCenter());
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

void AOctree::SetUpOctree()
{
	float MaxSize = FMath::Max3(ExpandVolumeXAxis, ExpandVolumeYAxis, ExpandVolumeZAxis) * SingleVolumeSize;
	//Add a little bit of padding, in case there is one single Octree underneath, which sometimes prevent FindNode to work properly.
	MaxSize *= 1.02f;

	RootNodeSharedPtr = MakeShareable(new OctreeNode(GetActorLocation(), MaxSize / 2));
	RootNodeSharedPtr->Occupied = true;

	TArray<FOverlapResult> Result;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);

	if (!ActorsToIgnore.IsEmpty())
	{
		for (const auto Actors : ActorsToIgnore)
		{
			TraceParams.AddIgnoredActor(Actors);
		}
	}

	TArray<FBox> BoxResults;

	if (!AutoEncapsulateObjects)
	{
		int Index = 0;
		RootNodeSharedPtr->ChildrenOctreeNodes.SetNum(ExpandVolumeXAxis * ExpandVolumeYAxis * ExpandVolumeZAxis);

		for (int X = 0; X < ExpandVolumeXAxis; X++)
		{
			for (int Y = 0; Y < ExpandVolumeYAxis; Y++)
			{
				for (int Z = 0; Z < ExpandVolumeZAxis; Z++)
				{
					TArray<FOverlapResult> ChildOverlaps;
					const FVector Offset = FVector(X * SingleVolumeSize, Y * SingleVolumeSize, Z * SingleVolumeSize);
					RootNodeSharedPtr->ChildrenOctreeNodes[Index] = MakeShareable(new OctreeNode(GetActorLocation() + Offset, SingleVolumeSize / 2));
					GetWorld()->OverlapMultiByChannel
					(
						ChildOverlaps,
						RootNodeSharedPtr->ChildrenOctreeNodes[Index]->Position,
						FQuat::Identity,
						CollisionChannel,
						FCollisionShape::MakeBox(FVector(SingleVolumeSize / 2)),
						TraceParams
					);
					Index++;

					//TODO make arrays of arrays instead of one big, then modify findandlode that looks at child rootnode specifically, saving time
					//in the begininng it scopes down to a single child root node so we know the index of which box array we would look at.
					Result.Append(ChildOverlaps);
				}
			}
		}
	}
	else
	{
		GetWorld()->OverlapMultiByChannel
		(
			Result,
			RootNodeSharedPtr->Position,
			FQuat::Identity, CollisionChannel,
			FCollisionShape::MakeBox(FVector(SingleVolumeSize / 2)),
			TraceParams
		);
	}


	for (const auto Overlap : Result)
	{
		if (Overlap.GetActor()->ActorHasTag(OctreeIgnoreTag)) continue;

		BoxResults.Add(Overlap.GetActor()->GetComponentsBoundingBox());
	}

	PathfindingWorker = MakeShareable(new FPathfindingWorker(RootNodeSharedPtr, Debug, BoxResults, MinNodeSize));
}
