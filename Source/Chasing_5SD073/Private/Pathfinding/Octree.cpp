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

void AOctree::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	
	for (const auto Element : EmptyLeaves)
	{

		//delete Element;
		if (Element != nullptr)
		{
			//UE_LOG(LogTemp, Display, TEXT("Meow: %f"), Element->MinSize);
		}
	}


	delete RootNodes[0];
	EmptyLeaves.Empty();
	//delete NavigationGraph;
	UE_LOG(LogTemp, Warning, TEXT("Nav graph elements: %i"), NavigationGraph->Nodes.Num());
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
	NavigationGraph->AddRootNode(NewRootNode);
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
		}
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
			NavigationGraph->AddNode(Node->GraphNode);
			EmptyLeaves.Add(Node);
			//DrawDebugBox(GetWorld(), Node->NodeBounds.GetCenter(), Node->NodeBounds.GetExtent(), FColor::Green, false, 15, 0, 3);
		}
		else
		{
			//If it has no children and contains an object, it is an unusable node and no longer needed. ~Occupied Node.
			//delete Node;
		}
	}
	else
	{
		for (const auto Element : Node->ChildrenOctreeNodes)
		{
			GetEmptyNodes(Element);
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

			if (!NavigationGraph->Nodes[i]->Neighbors.Contains(NavigationGraph->Nodes[j]) && DoNodesShareFace(EmptyLeaves[i], EmptyLeaves[j], 0.01f))
			{
				//DrawDebugLine(GetWorld(), EmptyLeaves[i]->NodeBounds.GetCenter(), EmptyLeaves[j]->NodeBounds.GetCenter(), FColor::Red, false, 15, 0,
				//              5);
				NavigationGraph->Nodes[i]->Neighbors.Add(EmptyLeaves[j]->GraphNode);
				NavigationGraph->Nodes[j]->Neighbors.Add(EmptyLeaves[i]->GraphNode);
			}
		}
	}

	//If by any chance there's a Node without a neighbor, we dont need it. (How would you go if no neighbor?)
	for (const auto Node : NavigationGraph->Nodes)
	{
		if (Node->Neighbors.IsEmpty())
		{
			delete Node;
		}
	}
}


bool AOctree::DoNodesShareFace(const OctreeNode* Node1, const OctreeNode* Node2, const float Tolerance)
{
	//In general, I am trying to nodes that share a face, no corners or edges. Even though the flying object could fly that way, it does not ensure that
	//it can 'slip' through that, wheres, due to the MinSize, a face-to-face movements will always ensure movement.
	//Additionally, regardless of checking corners/edges or not, the path smoothing at the end will provide the diagonal movement.
	//Therefore I save bunch of unnecessary checking in AStar or ConnectNode() that would have been cleaned up anyway.

	if (Node1 == nullptr || Node2 == nullptr) return false;

	const FBox& Box1 = Node1->NodeBounds;
	const FBox& Box2 = Node2->NodeBounds;

	// Check if the X faces are overlapping or adjacent
	const bool ShareXFace = FMath::Abs(Box1.Max.X - Box2.Min.X) < Tolerance || FMath::Abs(Box1.Min.X - Box2.Max.X) < Tolerance;

	// Check if the Y faces are overlapping or adjacent
	const bool ShareYFace = FMath::Abs(Box1.Max.Y - Box2.Min.Y) < Tolerance || FMath::Abs(Box1.Min.Y - Box2.Max.Y) < Tolerance;

	// Check if the Z faces are overlapping or adjacent
	const bool ShareZFace = FMath::Abs(Box1.Max.Z - Box2.Min.Z) < Tolerance || FMath::Abs(Box1.Min.Z - Box2.Max.Z) < Tolerance;


	if ((ShareXFace && !ShareYFace && !ShareZFace) || (!ShareXFace && ShareYFace && !ShareZFace) || (!ShareXFace && !ShareYFace && ShareZFace))
	{
		//The above conditions still returns true if the nodes share a face but are wide apart on one of the two other axis. Hence this
		if (FVector::Dist(Node1->NodeBounds.GetCenter(), Node2->NodeBounds.GetCenter()) <= Node1->NodeBounds.GetSize().X)
		{
			return true;
		}
	}
	return false;
}


bool AOctree::GetAStarPath(const FVector& Start, const FVector& End, FVector& NextLocation)
{
	NextLocation = FVector::ZeroVector;

	if (FVector::Distance(Start, End) <= 5.0f)
	{
		return false;
	}

	TArray<FVector> OutPath;

	if (NavigationGraph->Nodes.IsEmpty()) return false;

	if (NavigationGraph->OctreeAStar(Start, End, OutPath))
	{
		NextLocation = OutPath[0];
		for (int i = 0; i < OutPath.Num(); i++)
		{
			//DrawDebugSphere(GetWorld(), OutPath[i], 30, 4, FColor::Red, false, 15, 0, 5);
		}
		return true;
	}

	return false;
}
