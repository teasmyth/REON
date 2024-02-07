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
	const float Tolerance = 0.01f;

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
			const FVector NeighborDir = (Direction * Node->Bounds.GetSize().X / 2.0f) * 1.01f; //x1.01 so it is just inside the other cube
			const FVector NeighborLocation = Node->Bounds.GetCenter() + NeighborDir;
			OctreeGraphNode* PotentialNeighbor = FindGraphNode(NeighborLocation);

			if (PotentialNeighbor != nullptr && !Node->Neighbors.Contains(PotentialNeighbor) && Node->Bounds.Intersect(PotentialNeighbor->Bounds)) //&& DoNodesShareFace(Node, PotentialNeighbor, Tolerance))
			{
				Node->Neighbors.Add(PotentialNeighbor);
				PotentialNeighbor->Neighbors.Add(Node);
				//continue;
			}
		}
	}
}

bool OctreeGraph::DoNodesShareFace(const OctreeGraphNode* Node1, const OctreeGraphNode* Node2, const float Tolerance)
{
	//In general, I am trying to nodes that share a face, no corners or edges. Even though the flying object could fly that way, it does not ensure that
	//it can 'slip' through that, wheres, due to the MinSize, a face-to-face movements will always ensure movement.
	//Additionally, regardless of checking corners/edges or not, the path smoothing at the end will provide the diagonal movement.
	//Therefore I save bunch of unnecessary checking in AStar or ConnectNode() that would have been cleaned up anyway.

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
		if (FVector::Dist(Box1.GetCenter(), Box2.GetCenter()) <= Box1.GetSize().X)
		{
			return true;
		}
		*/
		return true;
	}
	return false;
}

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
	Start->H = ManhattanDistance(Start, End);
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
			Neighbor->H = ManhattanDistance(Neighbor, End);
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
	OutPathList.Empty();
	OutPathList.Add(End->Bounds.GetCenter());

	//Because I am using NodeBounds Center, adding those to the list might not ensure a smooth path.
	//For example, if a tiny node is a neighbor of a big one, the between them will be diagonal, which actually might not be free.
	//Thus, I am adding 'buffer' FVectors to ensure smooth path.

	if (Start == End)
	{
		return;
	}

	const OctreeGraphNode* CameFrom = End->CameFrom;
	while (CameFrom != nullptr && CameFrom != Start)
	{
		OutPathList.Insert(CameFrom->Bounds.GetCenter(), 0);
		CameFrom = CameFrom->CameFrom;
	}

	//OutPathList.Insert(Start->Bounds.GetCenter(), 0);
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

	if (CurrentNode != nullptr)
	{
		return CurrentNode->GraphNode;
	}

	UE_LOG(LogTemp, Warning, TEXT("Couldn't find node."));
	return nullptr;
}
