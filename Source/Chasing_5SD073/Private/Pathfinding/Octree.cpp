// Fill out your copyright notice in the Description page of Project Settings.


#include "Pathfinding/Octree.h"


AOctree::AOctree()
{
}

void AOctree::BeginPlay()
{
	Super::BeginPlay();

	RootNodes.Empty();

	NavigationGraph = new OctreeGraph();
	for (int X = 0; X < ExpandVolumeXAxis; X++)
	{
		for (int Y = 0; Y < ExpandVolumeYAxis; Y++)
		{
			const FVector Offset = FVector(X * SingleVolumeSize, Y * SingleVolumeSize, 0);
			MakeOctree(GetActorLocation() + Offset);
		}
	}

	const FBox DrawBox = FBox(RootNodes[0]->NodeBounds.Min, RootNodes[RootNodes.Num() - 1]->NodeBounds.Max);	
	DrawDebugBox(GetWorld(), DrawBox.GetCenter(), DrawBox.GetExtent(), FQuat::Identity, FColor::Blue, false, 15, 0, 10);


	const double StartTime = FPlatformTime::Seconds();
	NavigationGraph->ConnectNodes();
	//UE_LOG(LogTemp, Warning, TEXT("Connecting neighboring graph nodes took this many seconds: %lf"), FPlatformTime::Seconds() - StartTime);
	//UE_LOG(LogTemp, Warning, TEXT("Total Nodes: %i"), NavigationGraph->Nodes.Num());
	IsSetup = true;
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
	//Here where I will visualize the size
}

void AOctree::MakeOctree(const FVector& Origin)
{
	TArray<FOverlapResult> Result;
	const FVector Size = FVector(SingleVolumeSize);
	const FBox Bounds = FBox(Origin, Origin + Size);

	FCollisionQueryParams TraceParams;
	if (ActorToIgnore != nullptr)
	{
		TraceParams.AddIgnoredActor(ActorToIgnore);
	}

	GetWorld()->OverlapMultiByChannel(Result, Bounds.GetCenter(), FQuat::Identity, ECC_GameTraceChannel1, FCollisionShape::MakeBox(Size / 2.0f),
	                                  TraceParams);
	
	OctreeNode* NewRootNode = new OctreeNode(Bounds, nullptr);
	RootNodes.Add(NewRootNode);
	NavigationGraph->AddRootNode(NewRootNode);
	double StartTime = FPlatformTime::Seconds();
	AddObjects(Result, NewRootNode);
	//UE_LOG(LogTemp, Warning, TEXT("Generating nodes took this many seconds: %lf"), FPlatformTime::Seconds() - StartTime);
	StartTime = FPlatformTime::Seconds();
	GetEmptyNodes(NewRootNode);
	//UE_LOG(LogTemp, Warning, TEXT("Generating navigation graph took this many seconds: %lf"), FPlatformTime::Seconds() - StartTime);
	StartTime = FPlatformTime::Seconds();
	AdjustChildNodes(NewRootNode);
	//UE_LOG(LogTemp, Warning, TEXT("Cleaning up uncessary nodes took this many seconds: %lf"), FPlatformTime::Seconds() - StartTime);
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
		//DrawDebugBox(GetWorld(),Hit.GetActor()->GetComponentsBoundingBox().GetCenter(), Hit.GetActor()->GetComponentsBoundingBox().GetExtent(), FColor::Red, false, 15, 0 , 10);
	}
}

void AOctree::GetEmptyNodes(OctreeNode* Node)
{
	if (Node == nullptr)
	{
		return;
	}

	if (Node->ChildrenOctreeNodes.IsEmpty())
	{
		if (Node->ContainedActors.IsEmpty())
		{
			OctreeGraphNode* NewGraphNode = new OctreeGraphNode(Node->NodeBounds);
			Node->GraphNode = NewGraphNode;
			NavigationGraph->AddNode(NewGraphNode);
			//DrawDebugBox(GetWorld(), Node->NodeBounds.GetCenter(), Node->NodeBounds.GetExtent(), FColor::Green, false, 15, 0, 3);
		}
		else
		{
			const int Index = Node->Parent->ChildrenOctreeNodes.Find(Node);
			Node->Parent->ChildrenOctreeNodes[Index] = nullptr;
			delete Node;
			//I adjust the null pointers in AdjustChildNodes(). Here I should not touch it while it is iterating through it.
		}
	}
	else
	{
		for (const auto Child : Node->ChildrenOctreeNodes)
		{
			GetEmptyNodes(Child);
		}
	}
}

void AOctree::AdjustChildNodes(OctreeNode* Node)
{
	if (Node == nullptr)
	{
		return;
	}

	TArray<OctreeNode*> CleanedChildNodes;
	for (auto Child : Node->ChildrenOctreeNodes)
	{
		if (Child != nullptr)
		{
			CleanedChildNodes.Add(Child);
		}
	}

	Node->ChildrenOctreeNodes = CleanedChildNodes;

	for (const auto CleanedChildNode : CleanedChildNodes)
	{
		AdjustChildNodes(CleanedChildNode);
	}
}

bool AOctree::GetAStarPath(const FVector& Start, const FVector& End, FVector& NextLocation) const
{
	NextLocation = FVector::ZeroVector;

	if (FVector::Distance(Start, End) <= 5.0f)
	{
		return false;
	}

	TArray<FVector> OutPath;

	if (!IsSetup) return false;

	if (NavigationGraph->OctreeAStar(Start, End, OutPath))
	{
		NextLocation = OutPath[0];
		return true;
	}

	return false;
}
