// Fill out your copyright notice in the Description page of Project Settings.
#include "NavSystemVolume.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "NavSystemNode.h"
#include "DrawDebugHelpers.h"
#include "NavSystemComponent.h"
#include "NavSystemVolumeManager.h"
#include "queue"
#include "Async/Async.h"
#include "Kismet/GameplayStatics.h"

static UMaterial* GridMaterial = nullptr;

// Sets default values
ANavSystemVolume::ANavSystemVolume()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(false);

	// Create the default scene component
	DefaultSceneComponent = CreateDefaultSubobject<USceneComponent>("DefaultSceneComponent");
	SetRootComponent(DefaultSceneComponent);


	// Create the procedural mesh component

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>("ProceduralMesh");
	ProceduralMesh->SetupAttachment(GetRootComponent());
	ProceduralMesh->CastShadow = false;
	ProceduralMesh->SetEnableGravity(false);
	ProceduralMesh->bApplyImpulseOnDamage = false;
	//ProceduralMesh->SetGenerateOverlapEvents(false);
	ProceduralMesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	//ProceduralMesh->SetCollisionProfileName("NoCollision");
	ProceduralMesh->bHiddenInGame = false;
	ProceduralMesh->bUseComplexAsSimpleCollision = false;

	ProceduralMesh->SetMaterial(0, nullptr);


	// By default, hide the volume while the game is running
	//AActor::SetActorHiddenInGame(true); //todo check that its not bugging out, idk, the AActor:: was recommended by Rider.

	// Find and save the grid material
	static ConstructorHelpers::FObjectFinder<UMaterial> materialFinder(TEXT("Material'/Navigation3D/M_Grid.M_Grid'"));
	GridMaterial = materialFinder.Object;
}

// Called when the game starts or when spawned
void ANavSystemVolume::BeginPlay()
{
	Super::BeginPlay();

	ProceduralMesh->OnComponentBeginOverlap.AddDynamic(this, &ANavSystemVolume::OnOverlapBegin);
	ProceduralMesh->OnComponentEndOverlap.AddDynamic(this, &ANavSystemVolume::OnOverlapEnd);

	//Defining the colliders borders
	const TArray ConvexVerts = {
		FVector(GetGridSizeX(), 0, GetGridSizeZ()),
		FVector(0, 0, GetGridSizeZ()),
		FVector(0, 0, 0),
		FVector(GetGridSizeX(), 0, 0),
		FVector(GetGridSizeX(), GetGridSizeY(), GetGridSizeZ()),
		FVector(0, GetGridSizeY(), GetGridSizeZ()),
		FVector(0, GetGridSizeY(), 0),
		FVector(GetGridSizeX(), GetGridSizeY(), 0)
	};

	ProceduralMesh->ClearCollisionConvexMeshes();
	ProceduralMesh->AddCollisionConvexMesh(ConvexVerts);
}

void ANavSystemVolume::PopulateNodesAsync()
{
	//Because of how player is setup, it sometimes triggers twice.
	if (bAreNodesLoaded) return;

	Nodes = new NavSystemNode[GetTotalDivisions()];

	for (int32 z = 0; z < DivisionsZ; ++z)
	{
		for (int32 y = 0; y < DivisionsY; ++y)
		{
			for (int32 x = 0; x < DivisionsX; ++x)
			{
				const int32 divisionPerLevel = DivisionsX * DivisionsY;
				const int32 index = (z * divisionPerLevel) + (y * DivisionsX) + x;
				NavSystemNode* node = &Nodes[index];
				node->Coordinates = FIntVector(x, y, z);
			}
		}
	}
	bAreNodesLoaded = true;
}

void ANavSystemVolume::UnloadGridAsync()
{
	// Run the function asynchronously using a Lambda function
	FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this]()
	{
		// This code will run asynchronously on a background thread
		delete[] Nodes;
		Nodes = nullptr;
	}, TStatId(), nullptr, ENamedThreads::AnyThread);

	// Ensure the task is completed before continuing
	FTaskGraphInterface::Get().WaitUntilTaskCompletes(Task);

	// This code will run on the game thread when the task is complete
	//Do stuff()
	SetActorTickEnabled(false);
	bAreNodesLoaded = false;
	bStartUnloading = false;
}


void ANavSystemVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Delete the nodes
	delete [] Nodes;
	Nodes = nullptr;


	Super::EndPlay(EndPlayReason);
}

// Called every frame
void ANavSystemVolume::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);


	if (bStartUnloading)
	{
		m_unloadTimer += DeltaSeconds;
		if (m_unloadTimer >= UnloadTimer)
		{
			UnloadGridAsync();
			GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, ("Unloading Async"));
		}
	}
}

void ANavSystemVolume::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                      int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (VolumeManager == nullptr) return;


	if (AI_AgentActor != nullptr && OtherActor == AI_AgentActor && VolumeManager->GetAI_AgentActorVolume() != this)
	{
		VolumeManager->SetAI_AgentActorVolume(this);
	}

	if (TargetActor != nullptr && OtherActor == TargetActor && VolumeManager->GetTargetActorVolume() != this)
	{
		VolumeManager->SetTargetActorVolume(this);
		TargetActorEnterLocation = TargetActor->GetActorLocation();

		if (!bAreNodesLoaded)
		{
			TFuture<void> AsyncTask = Async(EAsyncExecution::Thread, [&]() { PopulateNodesAsync(); });
		}
		bStartUnloading = false;
		m_unloadTimer = 0;
	}
}

void ANavSystemVolume::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                    int32 OtherBodyIndex)
{
	if (VolumeManager == nullptr) return;


	if (AI_AgentActor != nullptr && OtherActor == AI_AgentActor)
	{
		if (bAreNodesLoaded)
		{
			SetActorTickEnabled(true);
			bStartUnloading = true;
			m_unloadTimer = 0;
		}

		VolumeManager->SetAI_AgentActorVolume(nullptr);
	}

	if (AI_AgentActor != nullptr && TargetActor != nullptr && OtherActor == TargetActor)
	{
		VolumeManager->SetTargetActorVolume(nullptr);
		TargetActorEndLocation = TargetActor->GetActorLocation();
	}
}

void ANavSystemVolume::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (VolumeManager == nullptr)
	{
		VolumeManager = Cast<ANavSystemVolumeManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ANavSystemVolumeManager::StaticClass()));

		if (VolumeManager == nullptr)
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Red, "Volume Manager Missing!");
		}
	}
	else
	{
		if (VolumeManager->GetTargetActor() != nullptr) TargetActor = VolumeManager->GetTargetActor();
		if (VolumeManager->GetAI_AgentActor() != nullptr) AI_AgentActor = VolumeManager->GetAI_AgentActor()->GetOwner();
	}

	CreateBorders();
}


void ANavSystemVolume::CreateGrid()
{
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

			CreateLine(start, end, FVector::UpVector, vertices, triangles, LineThickness);
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

			CreateLine(start, end, FVector::UpVector, vertices, triangles, LineThickness);
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

			CreateLine(start, end, FVector::ForwardVector, vertices, triangles, LineThickness);
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
	UMaterialInstanceDynamic* DynamicMaterialInstance = UMaterialInstanceDynamic::Create(GridMaterial, this);
	DynamicMaterialInstance->SetVectorParameterValue("Color", Color);
	DynamicMaterialInstance->SetScalarParameterValue("Opacity", Color.A);
	ProceduralMesh->SetMaterial(0, DynamicMaterialInstance);
}

void ANavSystemVolume::CreateBorders()
{
	if (DrawParallelogram)
	{
		CreateCubeMesh(
			FVector(GetGridSizeX(), 0, GetGridSizeZ()),
			FVector(0, 0, GetGridSizeZ()),
			FVector(0, 0, 0),
			FVector(GetGridSizeX(), 0, 0),
			FVector(GetGridSizeX(), GetGridSizeY(), GetGridSizeZ()),
			FVector(0, GetGridSizeY(), GetGridSizeZ()),
			FVector(0, GetGridSizeY(), 0),
			FVector(GetGridSizeX(), GetGridSizeY(), 0)
		);
		return;
	}

	// Create arrays to store the vertices and the triangles
	TArray<FVector> vertices;
	TArray<int32> triangles;

	// Define variables for the start and end of the line
	FVector start = FVector::ZeroVector;
	FVector end = FVector::ZeroVector;

	for (int32 y = 0; y <= GetGridSizeY(); y += GetGridSizeY())
	{
		for (int32 z = 0; z <= GetGridSizeZ(); z += GetGridSizeZ())
		{
			start = FVector(0, y, z);
			end = FVector(GetGridSizeX(), y, z);
			CreateLine(start, end, FVector::UpVector, vertices, triangles,LineThickness);
		}
	}

	for (int32 x = 0; x <= GetGridSizeX(); x += GetGridSizeX())
	{
		for (int32 z = 0; z <= GetGridSizeZ(); z += GetGridSizeZ())
		{
			start = FVector(x, 0, z);
			end = FVector(x, GetGridSizeY(), z);
			CreateLine(start, end, FVector::UpVector, vertices, triangles, LineThickness);
		}
	}

	for (int32 x = 0; x <= GetGridSizeX(); x += GetGridSizeX())
	{
		for (int32 y = 0; y <= GetGridSizeY(); y += GetGridSizeY())
		{
			start = FVector(x, y, 0);
			end = FVector(x, y, GetGridSizeZ());
			CreateLine(start, end, FVector::ForwardVector, vertices, triangles, LineThickness);
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
	UMaterialInstanceDynamic* DynamicMaterialInstance = UMaterialInstanceDynamic::Create(GridMaterial, this);
	DynamicMaterialInstance->SetVectorParameterValue("Color", Color);
	DynamicMaterialInstance->SetScalarParameterValue("Opacity", Color.A);
	ProceduralMesh->SetMaterial(0, DynamicMaterialInstance);
}


void ANavSystemVolume::CreateCubeMesh(const FVector& Corner1, const FVector& Corner2,
                                      const FVector& Corner3, const FVector& Corner4, const FVector& Corner5, const FVector& Corner6,
                                      const FVector& Corner7, const FVector& Corner8)
{
	// Define vertices for the cube
	TArray<FVector> Vertices = {
		Corner1, Corner2, Corner3, Corner4, // Front face
		Corner5, Corner6, Corner7, Corner8 // Back face
	};

	// Define triangles (index references to vertices)
	TArray<int32> Triangles = {
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
	TArray<FVector2D> UVs = {
		FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1), FVector2D(0, 1), // Front face
		FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1), FVector2D(0, 1) // Back face
	};

	TArray<FColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	// Set the data in the procedural mesh component
	//MeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);


	// Set the material on the procedural mesh so it's color/opacity can be configurable
	UMaterialInstanceDynamic* DynamicMaterialInstance = UMaterialInstanceDynamic::Create(GridMaterial, this);
	DynamicMaterialInstance->SetVectorParameterValue("Color", Color);
	DynamicMaterialInstance->SetScalarParameterValue("Opacity", Color.A);
	ProceduralMesh->SetMaterial(0, DynamicMaterialInstance);
}

void ANavSystemVolume::DeleteGrid() const
{
	ProceduralMesh->ClearAllMeshSections();
	ProceduralMesh->SetMaterial(0, nullptr);
}


void ANavSystemVolume::CreateLine(const FVector& start, const FVector& end, const FVector& normal, TArray<FVector>& vertices,
                                  TArray<int32>& triangles, const float& LineThickness)
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
