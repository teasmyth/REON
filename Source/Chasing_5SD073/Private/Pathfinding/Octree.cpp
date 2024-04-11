// Fill out your copyright notice in the Description page of Project Settings.
#include "Pathfinding/Octree.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Serialization/BufferArchive.h"
#include "Pathfinding/OctreeGraph.h"
#include "Pathfinding/OctreePathfindingComponent.h"
#include "Serialization/ArchiveLoadCompressedProxy.h"
#include "Serialization/ArchiveSaveCompressedProxy.h"
#include "Serialization/LargeMemoryReader.h"
#include "Serialization/LargeMemoryWriter.h"


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

	SetupOctreesFuture = Async(EAsyncExecution::Thread, [&]()
	{
		Loading = true;
		double StartTime = FPlatformTime::Seconds();

		if (const bool Loaded = LoadNodesFromFile(SaveFileName); !Loaded)
		{
			//GEngine call is not thread safe - just to note.
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, "No file found. Creating new nodes");

			SetUpOctreesAsync(false);

			SaveNodesToFile(SaveFileName);
		}
		else
		{
			if (Debug) UE_LOG(LogTemp, Warning, TEXT("Loading: %f"), FPlatformTime::Seconds() - StartTime);
		}

		//StartTime = FPlatformTime::Seconds();
		//OctreeGraph::ConnectNodes(Loading, RootNodeSharedPtr);
		//if (Debug) UE_LOG(LogTemp, Warning, TEXT("Connect nodes time: %f"), FPlatformTime::Seconds() - StartTime);

		const TWeakPtr<OctreeNode> RootNodeWeakPtr = RootNodeSharedPtr;
		PathfindingWorker = new FPathfindingWorker(RootNodeWeakPtr, Debug, OctreeData.ToWeakPtr());
		IsSetup = true;
	});
}

#pragma region Saving/Loading Data

void AOctree::SaveNodesToFile(const FString& Filename)
{
	if (IsSetup) return; //If the game is running, don't save the nodes. Might lead to data corruption.

	FLargeMemoryWriter ToBinary = FLargeMemoryWriter(0, true);
	FIndexData IndexData;
	
	double StartTime = FPlatformTime::Seconds();
	OctreeNode::SaveNode(ToBinary, IndexData, RootNodeSharedPtr, false);
	if (Debug) UE_LOG(LogTemp, Warning, TEXT("Baking 1 Done. Time: %f"), FPlatformTime::Seconds() - StartTime);
	
	StartTime = FPlatformTime::Seconds();
	ToBinary.FlushCache();
	ToBinary.Seek(0);
	
	OctreeNode::SaveNode(ToBinary, IndexData, RootNodeSharedPtr, true);
	if(Debug) UE_LOG(LogTemp, Warning, TEXT("Baking 2 Done. Time: %f"), FPlatformTime::Seconds() - StartTime);

	DeleteOctreeNode(RootNodeSharedPtr);

	
	// Calculate the number of chunks needed and then save it into the compressed data
	TArray64<uint8> BinaryData;
	int64 NumChunks = 0;
	constexpr int32 MaxTArraySize = TNumericLimits<int32>::Max();
	BinaryData.Append(ToBinary.GetData(), ToBinary.TotalSize());
	/*
	NumChunks = BinaryData.Num() / MaxTArraySize;
	if (BinaryData.Num() % MaxTArraySize != 0)
	{
		NumChunks++;
	}
	
	TArray<TArray<uint8>> AllCompressedData;
	AllCompressedData.SetNum(NumChunks);
	//TArray<uint8> CompressedDataChunk;
	TArray<uint8> DataToCompress;

	for (int64 i = 0; i < NumChunks; i++)
	{
		const double ChunkStartTime = FPlatformTime::Seconds();
		if (Debug) UE_LOG(LogTemp, Warning, TEXT("Compressing chunk %lld/%lld."), i + 1, NumChunks);
		FArchiveSaveCompressedProxy Compressor = FArchiveSaveCompressedProxy(AllCompressedData[i],TEXT("ZLIB"));

		// Calculate the start and end indices for this chunk
		const int64 StartIndex = i * MaxTArraySize + i;
		const int64 EndIndex = FMath::Min((i + 1) * MaxTArraySize, BinaryData.Num());
		
		for (int64 j = StartIndex; j < EndIndex; j++)
		{
			DataToCompress.Add(BinaryData[j]);
		}
		
		Compressor << DataToCompress;

		if (Compressor.GetError())
		{
			if (Debug) UE_LOG(LogTemp, Error, TEXT("Failed to compress data."));
			return;
		}
		
		Compressor.Flush();
		DataToCompress.Empty();

		if(Debug) UE_LOG(LogTemp, Warning, TEXT("Compressed chunk %lld/%lld. Took %f seconds."), i + 1, NumChunks, FPlatformTime::Seconds() - ChunkStartTime);
	}

	// Write the compressed data to your output...
	if (Debug) UE_LOG(LogTemp, Warning, TEXT("Started Saving"));

	//I cant save TArray<TArray<uint8>> directly to file, so I need to stitch them together. However, I can't use TArray<uint8> because it has a limit of 2GB.
	//I also cannot use TArray64, as it will try to allocate all the memory at once, which will crash the editor.
	TArray64<uint8> StitchedAllCompressedData;
	for (const auto& CompressedData : AllCompressedData)
	{
		// Store the size of the chunk
		int64 ChunkSize = CompressedData.Num();
		for (int i = 0; i < sizeof(int64); ++i)
		{
			StitchedAllCompressedData.Add((ChunkSize >> (i * 8)) & 0xFF);
		}

		// Store the chunk data
		StitchedAllCompressedData.Append(CompressedData);
	}
	*/

	//if (FFileHelper::SaveArrayToFile(StitchedAllCompressedData, *Filename))
	if (FFileHelper::SaveArrayToFile(BinaryData, *Filename))
	{
		if (Debug) UE_LOG(LogTemp, Warning, TEXT("Saved octree"));
	}
	else
	{
		if (Debug) UE_LOG(LogTemp, Error, TEXT("Couldn't save octree. Check the file path and/or permissions."));
	}

	/*
	FLargeMemoryWriter ToBinary = FLargeMemoryWriter(0, true);
	FIndexData IndexData;

	double StartTime = FPlatformTime::Seconds();
	OctreeNode::SaveNode(ToBinary, IndexData, RootNodeSharedPtr, false);
	UE_LOG(LogTemp, Warning, TEXT("Baking 1 Done. Time: %f"), FPlatformTime::Seconds() - StartTime);

	StartTime = FPlatformTime::Seconds();
	ToBinary.FlushCache();
	ToBinary.Seek(0);
	OctreeNode::SaveNode(ToBinary, IndexData, RootNodeSharedPtr, true);
	UE_LOG(LogTemp, Warning, TEXT("Baking 2 Done. Time: %f"), FPlatformTime::Seconds() - StartTime);


	//ToBinary << RootNodeSharedPtr;

	TArray64<uint8> BinaryData;
	BinaryData.Append(ToBinary.GetData(), ToBinary.TotalSize());

	// Determine the maximum size of a TArray<uint8>
	const int32 MaxTArraySize = TNumericLimits<int32>::Max();

	// Calculate the number of chunks needed
	int64 NumChunks = BinaryData.Num() / MaxTArraySize;
	if (BinaryData.Num() % MaxTArraySize != 0)
	{
		NumChunks++;
	}
	
	TArray<uint8> AllCompressedData;
	// Save the number of chunks
	TArray<uint8> CompressedNumChuck;
	FArchiveSaveCompressedProxy NumChuckCompressor = FArchiveSaveCompressedProxy(CompressedNumChuck,TEXT("ZLIB"));
	NumChuckCompressor << NumChunks;
	AllCompressedData.Append(CompressedNumChuck);

	for (int64 i = 0; i < NumChunks; i++)
	{
		const double ChunkStartTime = FPlatformTime::Seconds();
		// Calculate the start and end indices for this chunk
		const int64 StartIndex = i * MaxTArraySize;
		const int64 EndIndex = FMath::Min((i + 1) * MaxTArraySize, BinaryData.Num());

		// Create a TArray<uint8> from this chunk of the TArray64<uint8>
		TArray<uint8> DataToCompress;
		for (int64 j = StartIndex; j < EndIndex; j++)
		{
			DataToCompress.Add(BinaryData[j]);
		}

		// Compress the TArray<uint8>
		TArray<uint8> CompressedData;
		FArchiveSaveCompressedProxy Compressor = FArchiveSaveCompressedProxy(CompressedData,TEXT("ZLIB"));
		Compressor << DataToCompress;
		Compressor.Flush();

		AllCompressedData.Append(CompressedData);
		UE_LOG(LogTemp, Warning, TEXT("Compressed chunk %lld/%lld. Took %f seconds."), i + 1, NumChunks, FPlatformTime::Seconds() - ChunkStartTime);
	}

	// Write the compressed data to your output...
	UE_LOG(LogTemp, Warning, TEXT("Started Saving"));

	if (FFileHelper::SaveArrayToFile(AllCompressedData, *Filename))
	{
		UE_LOG(LogTemp, Warning, TEXT("Saved octree"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Couldn't save octree. Check the file path and/or permissions."));
	}
	*/

	/*
	TArray<uint8> BinaryData;
	BinaryData.Append(ToBinary.GetData(), ToBinary.TotalSize());
	
	TArray<uint8> CompressedData;
	StartTime = FPlatformTime::Seconds();
	FArchiveSaveCompressedProxy Compressor = FArchiveSaveCompressedProxy(CompressedData, TEXT("ZLIB"));

	Compressor << BinaryData;
	Compressor.Flush();
	UE_LOG(LogTemp, Warning, TEXT("Compressing done. Time: %f"), FPlatformTime::Seconds() - StartTime);

	if (Compressor.GetError())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to compress data."));
		return;
	}


	if (FFileHelper::SaveArrayToFile(CompressedData, *Filename))
	{
		UE_LOG(LogTemp, Warning, TEXT("Saved octree successfully."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Couldn't save octree. Check the file path and/or permissions."));
	}

	Compressor.FlushCache();
	CompressedData.Empty();

*/
	ToBinary.FlushCache();
	ToBinary.Close();
	BinaryData.Empty();

	if (Debug) UE_LOG(LogTemp, Warning, TEXT("Saving exited."));
}

bool AOctree::LoadNodesFromFile(const FString& Filename)
{
	/*
	TArray<uint8> CompressedData;

	if (FFileHelper::LoadFileToArray(CompressedData, *Filename))
	{
		if (CompressedData.Num() <= 0)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to deserialize octree nodes from file."));
			return false;
		}

		FArchiveLoadCompressedProxy Decompressor = FArchiveLoadCompressedProxy(CompressedData, TEXT("ZLIB"));

		if (Decompressor.GetError())
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to decompress file."));
			return false;
		}
		

		Decompressor << DecompressedBinaryArray;
		OctreeData = MakeShareable(new FLargeMemoryReader(DecompressedBinaryArray.GetData(), DecompressedBinaryArray.Num(),
		                                                  ELargeMemoryReaderFlags::None));
		OctreeData->Seek(0);
		RootNodeSharedPtr = OctreeNode::LoadSingleNode(*OctreeData, 0);
		//OctreeNode::LoadAllNodes( *OctreeData, RootNodeSharedPtr);
		OctreeGraph::RootNodeIndexData = OctreeNode::GetFIndexData(*OctreeData, 0);
		UOctreePathfindingComponent::OctreeData = OctreeData.Get();

		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, "Save File found. Loading nodes, skipping create");

		CompressedData.Empty();
		Decompressor.FlushCache();

		return true;
	}
	*/

	//TArray64<uint8> StitchedAllCompressedData;

	//if (FFileHelper::LoadFileToArray(StitchedAllCompressedData, *Filename))
	if (FFileHelper::LoadFileToArray(DecompressedBinaryArray, *Filename))
	{
		//if (StitchedAllCompressedData.Num() <= 0)
		if (DecompressedBinaryArray.Num() <= 0)
		{
			if (Debug) UE_LOG(LogTemp, Error, TEXT("Failed to deserialize octree nodes from file."));
			return false;
		}
		if (Debug) UE_LOG(LogTemp, Warning, TEXT("File loaded into array."))

		/*
		TArray<TArray<uint8>> AllCompressedData;
		for (int64 i = 0; i < StitchedAllCompressedData.Num();)
		{
			// Retrieve the size of the chunk
			int64 ChunkSize = 0;
			for (int j = 0; j < sizeof(int64); ++j)
			{
				ChunkSize |= static_cast<int64>(StitchedAllCompressedData[i++]) << (j * 8);
			}

			// Retrieve the chunk data
			TArray<uint8> CompressedData;
			for (int64 j = 0; j < ChunkSize; ++j)
			{
				CompressedData.Add(StitchedAllCompressedData[i++]);
			}
			AllCompressedData.Add(CompressedData);
		}
		
		
		for (int64 i = 0; i < AllCompressedData.Num(); i++)
		{
			if (Debug) UE_LOG(LogTemp, Warning, TEXT("Decompressing chunk %lld/%i."), i + 1, AllCompressedData.Num());
			const double ChunkStartTime = FPlatformTime::Seconds();
			TArray<uint8> DecompressedChunk;
			FArchiveLoadCompressedProxy Decompressor = FArchiveLoadCompressedProxy(AllCompressedData[i], TEXT("ZLIB"));

			if (Decompressor.IsError())
			{
				if(Debug)UE_LOG(LogTemp, Error, TEXT("Failed to decompress file."));
				return false;
			}
			
			if (AllCompressedData[i].Num() >= TNumericLimits<int32>::Max())
			{
				if (Debug) UE_LOG(LogTemp, Error, TEXT("Failed to decompress chunk %lld. Chunk size is larger than the maximum capacity of a TArray<uint8>."), i + 1);
				return false;
			}
			
			DecompressedChunk.Reserve(AllCompressedData[i].Num());
			Decompressor << DecompressedChunk;
			DecompressedBinaryArray.Append(DecompressedChunk);
			Decompressor.FlushCache();
			DecompressedChunk.Empty();
			if (Debug) UE_LOG(LogTemp, Warning, TEXT("Decompressed chunk %lld/%i. Took %f seconds."), i + 1, AllCompressedData.Num(), FPlatformTime::Seconds() - ChunkStartTime);
		}
		*/
		
		OctreeData = MakeShareable(new FLargeMemoryReader(DecompressedBinaryArray.GetData(), DecompressedBinaryArray.Num(), ELargeMemoryReaderFlags::None));
		if (Debug) UE_LOG(LogTemp, Warning, TEXT("Loaded into memory reader."));
		RootNodeSharedPtr = OctreeNode::LoadSingleNode(*OctreeData, 0);
		//OctreeNode::LoadAllNodes(*OctreeData, RootNodeSharedPtr);
		OctreeGraph::RootNodeIndexData = OctreeNode::GetFIndexData(*OctreeData, 0);
		UOctreePathfindingComponent::OctreeData = OctreeData.Get();

		if (Debug && GEngine) GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, "Save File found. Loading nodes, skipping create");

		//Uncomment it.
		//AllCompressedData.Empty();

		return true;
	}

	if (Debug) UE_LOG(LogTemp, Error, TEXT("Failed to load file into array."));
	return false;
}


#pragma endregion

void AOctree::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	IsSetup = false;
	Loading = false;

	SetupOctreesFuture.Wait();

	// Clean up
	if (PathfindingWorker != nullptr)
	{
		PathfindingWorker->Stop();
		PathfindingWorker->Exit();
		delete PathfindingWorker;
	}

	DeleteOctreeNode(RootNodeSharedPtr);


	if (OctreeData.IsValid())
	{
		OctreeData->Close();
		OctreeData.Reset();
		DecompressedBinaryArray.Empty();
	}
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
			if (!Child->Occupied)
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

	// Set the node's TSharedPtr to nullptr //Memory leak, doesnt seem to work.
	Node.Reset();
}

void AOctree::BakeOctree()
{
	if (!IsSetup)
	{
		UE_LOG(LogTemp, Warning, TEXT("Baking Octree"));
		SetUpOctreesAsync(false);
		const double StartTime = FPlatformTime::Seconds();
		OctreeGraph::ConnectNodes(true, RootNodeSharedPtr, RootNodeSharedPtr);
		UE_LOG(LogTemp, Warning, TEXT("Connect nodes time: %f"), FPlatformTime::Seconds() - StartTime);
		SaveNodesToFile(SaveFileName);
	}
	else UE_LOG(LogTemp, Warning, TEXT("Game is running, cant bake Octree."));
}

void AOctree::Shit()
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Green, "Andre is pooop.");
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

void AOctree::SetUpOctreesAsync(const bool IsLoading)
{
	FVector Size = FVector(ExpandVolumeXAxis * SingleVolumeSize, ExpandVolumeYAxis * SingleVolumeSize, ExpandVolumeZAxis * SingleVolumeSize);
	//Add a little bit of padding, in case there is one single Octree underneath, which sometimes prevent FindNode to work properly.
	Size *= 1.02f;

	RootNodeSharedPtr = MakeShareable(new OctreeNode(GetActorLocation(), SingleVolumeSize, false));
	RootNodeSharedPtr->Occupied = true;

	RootNodeSharedPtr->ChildrenOctreeNodes.SetNum(ExpandVolumeXAxis * ExpandVolumeYAxis * ExpandVolumeZAxis);

	if (IsLoading) return;

	double StartTime = FPlatformTime::Seconds();

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

	if (Debug) UE_LOG(LogTemp, Warning, TEXT("Octree setup time: %f"), FPlatformTime::Seconds() - StartTime);
	StartTime = FPlatformTime::Seconds();
	DeleteUnusedNodes(RootNodeSharedPtr);
	if (Debug) UE_LOG(LogTemp, Warning, TEXT("Empty nodes time: %f"), FPlatformTime::Seconds() - StartTime);
	StartTime = FPlatformTime::Seconds();
	AdjustDanglingChildNodes(RootNodeSharedPtr);
	if (Debug) UE_LOG(LogTemp, Warning, TEXT("Dangling nodes time: %f"), FPlatformTime::Seconds() - StartTime);
}

TSharedPtr<OctreeNode> AOctree::MakeOctree(const FVector& Origin, const int& Index)
{
	TSharedPtr<OctreeNode> NewRootNode = MakeShareable(new OctreeNode(Origin, SingleVolumeSize / 2.0f, false));

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


		GetWorld()->OverlapMultiByChannel(Result, NewRootNode->Position, FQuat::Identity, CollisionChannel,
		                                  FCollisionShape::MakeBox(FVector(SingleVolumeSize / 2.0f)), TraceParams);


		TArray<FBox> BoxResults;
		for (const auto Overlap : Result)
		{
			BoxResults.Add(Overlap.GetActor()->GetComponentsBoundingBox());
		}

		AddObjects(BoxResults, NewRootNode);
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

void AOctree::DeleteUnusedNodes(const TSharedPtr<OctreeNode>& Node)
{
	if (!Node.IsValid())
	{
		return;
	}
	
	/*
	TArray<TSharedPtr<OctreeNode>> NodeList;
	NodeList.Add(Node);

	for (int i = 0; i < NodeList.Num(); i++)
	{
		if (NodeList[i] == nullptr)
		{
			continue;
		}

		for (auto& Child : NodeList[i]->ChildrenOctreeNodes)
		{
			if (Child->ChildrenOctreeNodes.IsEmpty())
			{
				if (Child->Occupied)
				{
					Child.Reset();
					//I adjust the null pointers in AdjustDanglingChildNodes(). Here I should not touch it while it is iterating through it.
				}
			}
			else
			{
				NodeList.Add(Child);
			}
		}
	}*/

	for (auto& Child : Node->ChildrenOctreeNodes)
	{
		if (Child->ChildrenOctreeNodes.IsEmpty())
		{
			if (Child->Occupied)
			{
				Child.Reset();
				//I adjust the null pointers in AdjustDanglingChildNodes(). Here I should not touch it while it is iterating through it.
			}
		}
		else
		{
			DeleteUnusedNodes(Child);
		}
	}
}

void AOctree::AdjustDanglingChildNodes(const TSharedPtr<OctreeNode>& Node)
{
	if (!Node.IsValid())
	{
		return;
	}

	/*
	TArray<TSharedPtr<OctreeNode>> NodeList;
	NodeList.Add(Node);

	for (int i = 0; i < NodeList.Num(); i++)
	{
		if (NodeList[i] == nullptr) continue;

		const TSharedPtr<OctreeNode> CurrentNode = NodeList[i];

		TArray<TSharedPtr<OctreeNode>> CleanedChildNodes;
		for (auto Child : CurrentNode->ChildrenOctreeNodes)
		// todo it is not const auto &, maybe thats why there were child nodes with 0 child but occupied.
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
	*/
	TArray<TSharedPtr<OctreeNode>> CleanedChildNodes;
	for (const auto& Child : Node->ChildrenOctreeNodes)
	{
		if (Child.IsValid())
		{
			CleanedChildNodes.Add(Child);
		}

		AdjustDanglingChildNodes(Child);
	}
	Node->ChildrenOctreeNodes = CleanedChildNodes;
}

#pragma endregion

#pragma region Pathfinding

#pragma endregion
