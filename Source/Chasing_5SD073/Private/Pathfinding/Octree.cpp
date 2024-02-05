// Fill out your copyright notice in the Description page of Project Settings.


#include "Pathfinding/Octree.h"

AOctree::AOctree()
{
}

void AOctree::BeginPlay()
{
	Super::BeginPlay();

	RootNodes.Empty();
	EmptyLeaves.Empty();

	NavigationGraph = new OctreeGraph();
	for (int X = 0; X < ExpandVolumeXAxis; X++)
	{
		for (int Y = 0; Y < ExpandVolumeYAxis; Y++)
		{
			const FVector Offset = FVector(X * SingleVolumeSize, Y * SingleVolumeSize, 0);
			MakeOctree(GetActorLocation() + Offset);
		}
	}

	ConnectNodes();
	UE_LOG(LogTemp, Warning, TEXT("Total Nodes: %i"), NavigationGraph->Nodes.Num());
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
	const FBox Bounds = FBox(Origin - Size / 2.0f, Origin + Size / 2.0f);

	FCollisionQueryParams TraceParams;
	if (ActorToIgnore != nullptr)
	{
		TraceParams.AddIgnoredActor(ActorToIgnore);
	}

	GetWorld()->OverlapMultiByChannel(Result, Origin, FQuat::Identity, ECC_GameTraceChannel1, FCollisionShape::MakeBox(Size / 2.0f), TraceParams);
	DrawDebugBox(GetWorld(), Bounds.GetCenter(), Bounds.GetExtent() * 1.01f, FQuat::Identity, FColor::Blue, false, 15, 0, 10);

	OctreeNode* NewRootNode = new OctreeNode(Bounds, MinNodeSize, nullptr);
	RootNodes.Add(NewRootNode);
	//UE_LOG(LogTemp, Warning, TEXT("Found objects: %i"), Result.Num());
	AddObjects(Result, NewRootNode);
	GetEmptyNodes(NewRootNode);
}

void AOctree::AddObjects(TArray<FOverlapResult> FoundObjects, OctreeNode* RootN) const
{
	UWorld* World = GetWorld();
	if (!FoundObjects.IsEmpty())
	{
		for (auto Hit : FoundObjects)
		{
			RootN->DivideNodeRecursively(Hit.GetActor(), World);
			//DrawDebugBox(World, Hit.GetActor()->GetComponentsBoundingBox().GetCenter(), Hit.GetActor()->GetComponentsBoundingBox().GetExtent(),
			//             FQuat::Identity, FColor::Red, false, 15, 0, 3);
		}
	}
	else
	{
		DrawDebugBox(GetWorld(), RootN->NodeBounds.GetCenter(), RootN->NodeBounds.GetExtent(), FQuat::Identity, FColor::Green, false, 15, 0, 10);
	}
}

void AOctree::GetEmptyNodes(OctreeNode* Node)
{
	if (Node == nullptr)
	{
		return;
	}

	NavigationGraph->AddRootNode(Node);

	if (Node->ChildrenOctreeNodes.IsEmpty())
	{
		if (Node->ContainedActors.IsEmpty())
		{
			NavigationGraph->AddNode(Node->GraphNode);
			EmptyLeaves.Add(Node);
			DrawDebugBox(GetWorld(), Node->NodeBounds.GetCenter(), Node->NodeBounds.GetExtent(),FColor::Green, false, 15, 0, 3);
		}
		else
		{
			//If it has no children and contains an object, it is an unusable node and no longer needed.
			delete Node;
		}
	}
	else
	{
		for (int i = 0; i < 8; i++)
		{
			GetEmptyNodes(Node->ChildrenOctreeNodes[i]);
		}
	}
}

void AOctree::ConnectNodes()
{
	for (int i = 0; i < EmptyLeaves.Num(); i++)
	{
		for (int j = 0; j < EmptyLeaves.Num(); j++)
		{
			if (i == j) continue;

			if (!NavigationGraph->Nodes[i]->Neighbors.Contains(NavigationGraph->Nodes[j]) && DoNodesShareFace(EmptyLeaves[i], EmptyLeaves[j]))
			{
				NavigationGraph->Nodes[i]->Neighbors.Add(EmptyLeaves[j]->GraphNode);
				NavigationGraph->Nodes[j]->Neighbors.Add(EmptyLeaves[i]->GraphNode);
			}
		}
	}
}

bool AOctree::DoNodesTouchOnAnyAxis(const OctreeNode* Node1, const OctreeNode* Node2)
{
	if (Node1 == nullptr || Node2 == nullptr) return false;

	const FBox& Box1 = Node1->NodeBounds;
	const FBox& Box2 = Node2->NodeBounds;

	return (Box1.Max.X == Box2.Min.X || Box1.Min.X == Box2.Max.X) ||
	   (Box1.Max.Y == Box2.Min.Y || Box1.Min.Y == Box2.Max.Y) ||
	   (Box1.Max.Z == Box2.Min.Z || Box1.Min.Z == Box2.Max.Z);
}

bool AOctree::DoNodesShareFace(const OctreeNode* Node1, const OctreeNode* Node2)
{
	if (Node1 == nullptr || Node2 == nullptr) return false;

	const FBox& Box1 = Node1->NodeBounds;
	const FBox& Box2 = Node2->NodeBounds;

	int equalAxesCount = 0;

	if (FMath::IsNearlyEqual(Box1.Max.X, Box2.Min.X) || FMath::IsNearlyEqual(Box1.Min.X, Box2.Max.X))
	{
		equalAxesCount++;
	}

	if (FMath::IsNearlyEqual(Box1.Max.Y, Box2.Min.Y) || FMath::IsNearlyEqual(Box1.Min.Y, Box2.Max.Y))
	{
		equalAxesCount++;
	}

	if (FMath::IsNearlyEqual(Box1.Max.Z, Box2.Min.Z) || FMath::IsNearlyEqual(Box1.Min.Z, Box2.Max.Z))
	{
		equalAxesCount++;
	}

	// Nodes share a face if at least two coordinates along each axis are equal
	return equalAxesCount >= 2;
}

bool AOctree::GetAStarPath(const FVector& Start, const FVector& End, FVector& NextLocation)
{
	//blah blah
	TArray<FVector> OutPath;
	NextLocation = FVector::ZeroVector;
	
	if (NavigationGraph->Nodes.IsEmpty()) return false;

	if (NavigationGraph->OctreeAStar(Start, End, OutPath))
	{
		NextLocation = OutPath[0];
		for (int i = 0; i < OutPath.Num(); i++)
		{
			DrawDebugSphere(GetWorld(), OutPath[i], 30, 4, FColor::Red, false, 15, 0, 5);
		}
		return true;
	}

	return false;
}
