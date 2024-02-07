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

	const double StartTime = FPlatformTime::Seconds();
	//ConnectNodes();
	NavigationGraph->ConnectNodes();
	UE_LOG(LogTemp, Warning, TEXT("Connecting neighboring graph nodes took this many seconds: %lf"), FPlatformTime::Seconds() - StartTime);

	
	for (auto Element : NavigationGraph->Nodes)
	{
		for (auto Neighbor : Element->Neighbors)
		{
			//DrawDebugLine(GetWorld(), Element->Bounds.GetCenter(), Neighbor->Bounds.GetCenter(), FColor::Red, false, 15, 0, 5);
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Total Nodes: %i"), NavigationGraph->Nodes.Num());
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
	DrawDebugBox(GetWorld(), Bounds.GetCenter(), Bounds.GetExtent(), FQuat::Identity, FColor::Blue, false, 15, 0, 10);

	OctreeNode* NewRootNode = new OctreeNode(Bounds, nullptr);
	RootNodes.Add(NewRootNode);
	NavigationGraph->AddRootNode(NewRootNode);
	double StartTime = FPlatformTime::Seconds();
	AddObjects(Result, NewRootNode);
	UE_LOG(LogTemp, Warning, TEXT("Generating nodes took this many seconds: %lf"), FPlatformTime::Seconds() - StartTime);
	StartTime = FPlatformTime::Seconds();
	GetEmptyNodes(NewRootNode);
	UE_LOG(LogTemp, Warning, TEXT("Generating navigation graph took this many seconds: %lf"), FPlatformTime::Seconds() - StartTime);
	StartTime = FPlatformTime::Seconds();
	AdjustChildNodes(NewRootNode);
	UE_LOG(LogTemp, Warning, TEXT("Cleaning up uncessary nodes took this many seconds: %lf"), FPlatformTime::Seconds() - StartTime);
}

void AOctree::AddObjects(TArray<FOverlapResult> FoundObjects, OctreeNode* RootN) const
{
	if (FoundObjects.IsEmpty())
	{
		return;
	}

	for (auto Hit : FoundObjects)
	{
		//RootN->DivideNodeRecursively(Hit.GetActor(), MinNodeSize);

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

void AOctree::ConnectNodes() const
{
	for (int i = 0; i < NavigationGraph->Nodes.Num(); i++)
	{
		for (int j = 0; j < NavigationGraph->Nodes.Num(); j++)
		{
			if (i == j) continue;

			//If we already checked, no point of checking again.
			if (NavigationGraph->Nodes[i]->Neighbors.Contains(NavigationGraph->Nodes[j])) continue;

			if (FVector::Dist(NavigationGraph->Nodes[i]->Bounds.GetCenter(), NavigationGraph->Nodes[j]->Bounds.GetCenter()) > NavigationGraph->Nodes[
				i]->Bounds.GetSize().X)
			{
				continue;
			}

			//If there are no neighbors, no point of continuing.
			if (!DoNodesShareFace(NavigationGraph->Nodes[i], NavigationGraph->Nodes[j], 0.01f)) continue;

			//DrawDebugLine(GetWorld(), NavigationGraph->Nodes[i]->Bounds.GetCenter(), NavigationGraph->Nodes[j]->Bounds.GetCenter(), FColor::Red,
			 //             false, 15, 0,5);
			NavigationGraph->Nodes[i]->Neighbors.Add(NavigationGraph->Nodes[j]);
			NavigationGraph->Nodes[j]->Neighbors.Add(NavigationGraph->Nodes[i]);
		}
	}

	//I used to have a loop here where I delete Nodes with no neighbors, but after extensive test, never encountered one.
	//(There shouldn't be one anyway in an Octree)
	//So, deleted it, as it was unnecessary.	
}


bool AOctree::DoNodesShareFace(const OctreeGraphNode* Node1, const OctreeGraphNode* Node2, const float Tolerance)
{
	//In general, I am trying to nodes that share a face, no corners or edges. Even though the flying object could fly that way, it does not ensure that
	//it can 'slip' through that, wheres, due to the MinSize, a face-to-face movements will always ensure movement.
	//Additionally, regardless of checking corners/edges or not, the path smoothing at the end will provide the diagonal movement.
	//Therefore I save bunch of unnecessary checking in AStar or ConnectNode() that would have been cleaned up anyway.

	//if (Node1 == nullptr || Node2 == nullptr) return false;

	const FBox& Box1 = Node1->Bounds;
	const FBox& Box2 = Node2->Bounds;

	// Check if the X faces are overlapping or adjacent
	const bool ShareXFace = FMath::Abs(Box1.Max.X - Box2.Min.X) <= Tolerance || FMath::Abs(Box1.Min.X - Box2.Max.X) <= Tolerance;

	// Check if the Y faces are overlapping or adjacent
	const bool ShareYFace = FMath::Abs(Box1.Max.Y - Box2.Min.Y) <= Tolerance || FMath::Abs(Box1.Min.Y - Box2.Max.Y) <= Tolerance;

	// Check if the Z faces are overlapping or adjacent
	const bool ShareZFace = FMath::Abs(Box1.Max.Z - Box2.Min.Z) <= Tolerance || FMath::Abs(Box1.Min.Z - Box2.Max.Z) <= Tolerance;


	if ((ShareXFace && !ShareYFace && !ShareZFace) || (!ShareXFace && ShareYFace && !ShareZFace) || (!ShareXFace && !ShareYFace && ShareZFace))
	{
		/*
		//The above conditions still returns true if the nodes share a face but are wide apart on one of the two other axis. Hence this
		if (FVector::Dist(Node1->Bounds.GetCenter(), Node2->Bounds.GetCenter()) <= Node1->Bounds.GetSize().X)
		{
			return true;
		}
		*/
		return true;
	}
	return false;
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
		for (int i = 0; i < OutPath.Num(); i++)
		{
			//DrawDebugSphere(GetWorld(), OutPath[i], 30, 4, FColor::Red, false, 15, 0, 5);
		}
		return true;
	}

	return false;
}
