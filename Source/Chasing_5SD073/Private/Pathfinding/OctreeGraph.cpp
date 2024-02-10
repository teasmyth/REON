// Fill out your copyright notice in the Description page of Project Settings.


#include "Pathfinding/OctreeGraph.h"

#include <queue>
#include <vector>

#include "Pathfinding/OctreeNode.h"

OctreeGraph::OctreeGraph()
{
	Nodes.Empty();
}

OctreeGraph::~OctreeGraph()
{
	for (const auto Element : Nodes)
	{
		delete Element;
	}

	Nodes.Empty();
}

void OctreeGraph::AddNode(OctreeGraphNode* Node)
{
	Nodes.Add(Node);
}

void OctreeGraph::AddRootNode(OctreeNode* Node)
{
	RootNodes.Add(Node);
}

void OctreeGraph::ConnectNodes()
{
	const TArray<FVector> Directions
	{
		FVector(1, 0, 0),
		FVector(-1, 0, 0),
		FVector(0, 1, 0),
		FVector(0, -1, 0),
		FVector(0, 0, 1),
		FVector(0, 0, -1)
	};

	for (const auto Node : Nodes)
	{
		for (auto Direction : Directions)
		{
			const FVector NeighborLocation = Node->Bounds.GetCenter() + Direction * Node->Bounds.GetSize().X;
			OctreeGraphNode* PotentialNeighbor = FindGraphNode(NeighborLocation);

			if (PotentialNeighbor != nullptr && !Node->Neighbors.Contains(PotentialNeighbor) && Node->Bounds.Intersect(PotentialNeighbor->Bounds))
			{
				Node->Neighbors.Add(PotentialNeighbor);
				PotentialNeighbor->Neighbors.Add(Node);
			}
		}
	}
}

static float ExtraHWeight = 2.0f;

bool OctreeGraph::OctreeAStar(const FVector& StartLocation, const FVector& EndLocation, TArray<FVector>& OutPathList)
{
	OctreeGraphNode* Start = FindGraphNode(StartLocation);
	const OctreeGraphNode* End = FindGraphNode(EndLocation);
	OutPathList.Empty();


	if (Start == nullptr || End == nullptr)
	{
		return false;
	}

	std::priority_queue<OctreeGraphNode*, std::vector<OctreeGraphNode*>, FOctreeGraphNodeCompare> OpenSet;
	TArray<OctreeGraphNode*> OpenList; //I use it to keep track of all the nodes used to reset them and also check which ones I checked before.
	TArray<OctreeGraphNode*> ClosedList;

	Start->G = 0;
	Start->H = ManhattanDistance(Start, End) * ExtraHWeight;
	Start->F = Start->H;

	OpenSet.push(Start);
	OpenList.Add(Start);

	while (!OpenSet.empty())
	{
		OctreeGraphNode* CurrentNode = OpenSet.top();

		if (CurrentNode == End)
		{
			ReconstructPath(Start, End, OutPathList);
			OutPathList.Add(EndLocation);
			//UE_LOG(LogTemp, Warning, TEXT("Path length: %i"), OutPathList.Num());

			for (const auto Node : OpenList)
			{
				Node->G = FLT_MAX;
				Node->F = FLT_MAX;
				Node->H = FLT_MAX;
				Node->CameFrom = nullptr;
			}
			return true;
		}

		OpenSet.pop();
		ClosedList.Add(CurrentNode);

		if (CurrentNode->Neighbors.IsEmpty()) continue;

		for (const auto Neighbor : CurrentNode->Neighbors)
		{
			if (ClosedList.Contains(Neighbor)) continue; //We already have the best route to that node.

			const float TentativeG = CurrentNode->G + ManhattanDistance(CurrentNode, Neighbor);

			if (Neighbor->G <= TentativeG) continue;

			Neighbor->G = TentativeG;
			Neighbor->H = ManhattanDistance(Neighbor, End) * ExtraHWeight; // Can do weighted to increase performance * 2 ;
			Neighbor->F = Neighbor->G + Neighbor->H;

			Neighbor->CameFrom = CurrentNode;

			if (!OpenList.Contains(Neighbor))
			{
				OpenList.Add(Neighbor);
				OpenSet.push(Neighbor);
			}
		}
	}
	UE_LOG(LogTemp, Error, TEXT("Couldn't find path"));
	return false;
}

void OctreeGraph::ReconstructPath(const OctreeGraphNode* Start, const OctreeGraphNode* End, TArray<FVector>& OutPathList)
{
	//Because I am using NodeBounds Center, adding those to the list might not ensure a smooth path.
	//For example, if a tiny node is a neighbor of a big one, the between them will be diagonal, which actually might not be free.
	//Thus, I am adding 'buffer' FVectors to ensure smooth path.
	//However, the physics based smoothing is done in Octree

	if (Start == End)
	{
		return;
	}

	OutPathList.Empty();
	OutPathList.Add(End->Bounds.GetCenter());

	const TArray<FVector> Directions
	{
		FVector(1, 0, 0),
		FVector(-1, 0, 0),
		FVector(0, 1, 0),
		FVector(0, -1, 0),
		FVector(0, 0, 1),
		FVector(0, 0, -1)
	};


	const OctreeGraphNode* CameFrom = End->CameFrom;
	const OctreeGraphNode* Previous = End;

	while (CameFrom != nullptr && CameFrom != Start)
	{
		if (Previous->Bounds.GetSize().X != CameFrom->Bounds.GetSize().X)
		{
			const FVector BufferVector = DirectionTowardsSharedFaceFromSmallerNode(Previous, CameFrom);
			OutPathList.Insert(BufferVector, 0);
		}
		
		OutPathList.Insert(CameFrom->Bounds.GetCenter(), 0);
		Previous = CameFrom;
		CameFrom = CameFrom->CameFrom;
	}
}

FVector OctreeGraph::DirectionTowardsSharedFaceFromSmallerNode(const OctreeGraphNode* Node1, const OctreeGraphNode* Node2)
{
	FBox SmallBounds;
	FVector SmallerCenter;
	FVector LargerCenter;

	if (Node1->Bounds.GetSize().X < Node2->Bounds.GetSize().X)
	{
		SmallBounds = Node1->Bounds;
		SmallerCenter = Node1->Bounds.GetCenter();
		LargerCenter = Node2->Bounds.GetCenter();
	}
	else if (Node2->Bounds.GetSize().X < Node1->Bounds.GetSize().X)
	{
		SmallBounds = Node2->Bounds;
		SmallerCenter = Node2->Bounds.GetCenter();
		LargerCenter = Node1->Bounds.GetCenter();
	}
	
	// Calculate the difference vector between the centers of the two boxes
	const FVector Delta = LargerCenter - SmallerCenter;

	// Find the axis with the maximum absolute component in the difference vector
	int MaxAxis = 0;

	float MaxComponent = FMath::Abs(Delta.X);

	if (FMath::Abs(Delta.Y) > MaxComponent)
	{
		MaxAxis = 1;
		MaxComponent = FMath::Abs(Delta.Y);
	}
	if (FMath::Abs(Delta.Z) > MaxComponent)
	{
		MaxAxis = 2;
	}

	// Construct the direction vector towards the shared face
	FVector Direction(0.0f);
	Direction[MaxAxis] = (Delta[MaxAxis] > 0) ? 1.0f : -1.0f;

	return SmallerCenter + Direction * SmallBounds.GetSize().X;
}

float OctreeGraph::ManhattanDistance(const OctreeGraphNode* From, const OctreeGraphNode* To)
{
	const FVector Point1 = From->Bounds.GetCenter();
	const FVector Point2 = To->Bounds.GetCenter();

	const float Dx = FMath::Abs(Point2.X - Point1.X);
	const float Dy = FMath::Abs(Point2.Y - Point1.Y);
	const float Dz = FMath::Abs(Point2.Z - Point1.Z);

	return Dx + Dy + Dz;
}

OctreeGraphNode* OctreeGraph::FindGraphNode(const FVector& Location)
{
	OctreeNode* CurrentNode = nullptr;

	//Handy if there are multiple roots
	for (const auto Root : RootNodes)
	{
		if (Root->NodeBounds.IsInside(Location))
		{
			CurrentNode = Root;
			break;
		}
	}

	if (CurrentNode == nullptr)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Node is not inside any of the roots."));
		return nullptr;
	}

	while (!CurrentNode->ChildrenOctreeNodes.IsEmpty())
	{
		OctreeNode* Closest = CurrentNode->ChildrenOctreeNodes[0];
		for (const auto Node : CurrentNode->ChildrenOctreeNodes)
		{
			if (Node->NodeBounds.IsInside(Location))
			{
				Closest = Node;
			}
		}
		CurrentNode = Closest;
	}

	if (CurrentNode != nullptr )//&& CurrentNode->NodeBounds.IsInside(Location))
	{
		return CurrentNode->GraphNode;
	}

	UE_LOG(LogTemp, Warning, TEXT("Couldn't find node."));
	return nullptr;
}