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
	Nodes.Empty();
}

void OctreeGraph::AddNode(OctreeNode* Node)
{
	Nodes.Add(Node);
}

void OctreeGraph::AddRootNode(OctreeNode* Node)
{
	RootNodes.Add(Node);
}

void OctreeGraph::ConnectNodes()
{
	for (const auto Node : Nodes)
	{
		for (auto Direction : Directions)
		{
			const FVector NeighborLocation = Node->NodeBounds.GetCenter() + Direction * Node->NodeBounds.GetSize().X;
			OctreeNode* PotentialNeighbor = FindGraphNode(NeighborLocation);

			if (PotentialNeighbor != nullptr  && !Node->Neighbors.Contains(PotentialNeighbor) && Node->NodeBounds.Intersect(PotentialNeighbor->NodeBounds))
			{
				Node->Neighbors.Add(PotentialNeighbor);
				PotentialNeighbor->Neighbors.Add(Node);
			}
		}
	}
}

//Can do weighted to increase performance * 2. Higher numbers should yield faster path finding but might sacrifice accuracy.
static float ExtraHWeight = 2.0f; 

bool OctreeGraph::OctreeAStar(const FVector& StartLocation, const FVector& EndLocation, TArray<FVector>& OutPathList)
{
	OctreeNode* Start = FindGraphNode(StartLocation);
	const OctreeNode* End = FindGraphNode(EndLocation);
	OutPathList.Empty();


	if (Start == nullptr || End == nullptr)
	{
		return false;
	}

	std::priority_queue<OctreeNode*, std::vector<OctreeNode*>, FOctreeNodeCompare> OpenSet;
	TArray<OctreeNode*> OpenList; //I use it to keep track of all the nodes used to reset them and also check which ones I checked before.
	TArray<OctreeNode*> ClosedList;

	Start->G = 0;
	Start->H = ManhattanDistance(Start, End) * ExtraHWeight;
	Start->F = Start->H;

	OpenSet.push(Start);
	OpenList.Add(Start);

	while (!OpenSet.empty())
	{
		OctreeNode* CurrentNode = OpenSet.top();

		if (CurrentNode == End)
		{
			ReconstructPath(Start, End, OutPathList);
			OutPathList.Add(EndLocation);

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

void OctreeGraph::ReconstructPath(const OctreeNode* Start, const OctreeNode* End, TArray<FVector>& OutPathList)
{
	/* Because I am using NodeBounds Center, adding those to the list might not ensure a smooth path.
	 * For example, if a tiny node is a neighbor of a big one, the line between them will be diagonal, which actually might not be unobstructed.
	 *
	 * ---------------------------------|
	 *|									|
	 *|									|
	 *|									|
	 *|			  						|
	 *|									|
	 *|					0				|
	 *|									|
	 *|									|
	 *|									|-----|<- Occupied
	 *|									|/////|					
	 *|									|-----|
	 *|				Buffer Vector -> X	|  0  | <- Free
	 *|---------------------------------|-----|
	 *
	 *
	 * If we are doing a sweeping from Big box to the small unoccupied box, the shape of the sweeping will definitely touch the occupied box
	 * So, by adding a buffer vector, we ensure there is a path.
	 * However, the physics based smoothing is done in Octree
	 */

	if (Start == End)
	{
		return;
	}

	OutPathList.Empty();
	OutPathList.Add(End->NodeBounds.GetCenter());

	const OctreeNode* CameFrom = End->CameFrom;
	const OctreeNode* Previous = End;

	while (CameFrom != nullptr && CameFrom != Start)
	{
		if (Previous->NodeBounds.GetSize().X != CameFrom->NodeBounds.GetSize().X)
		{
			const FVector BufferVector = DirectionTowardsSharedFaceFromSmallerNode(Previous, CameFrom);
			OutPathList.Insert(BufferVector, 0);
		}
		
		OutPathList.Insert(CameFrom->NodeBounds.GetCenter(), 0);
		Previous = CameFrom;
		CameFrom = CameFrom->CameFrom;
	}

	//OutPathList.Insert(Start->NodeBounds.GetCenter(),0);
}

FVector OctreeGraph::DirectionTowardsSharedFaceFromSmallerNode(const OctreeNode* Node1, const OctreeNode* Node2)
{
	FBox SmallBounds;
	FVector SmallerCenter;
	FVector LargerCenter;

	if (Node1->NodeBounds.GetSize().X < Node2->NodeBounds.GetSize().X)
	{
		SmallBounds = Node1->NodeBounds;
		SmallerCenter = Node1->NodeBounds.GetCenter();
		LargerCenter = Node2->NodeBounds.GetCenter();
	}
	else if (Node2->NodeBounds.GetSize().X < Node1->NodeBounds.GetSize().X)
	{
		SmallBounds = Node2->NodeBounds;
		SmallerCenter = Node2->NodeBounds.GetCenter();
		LargerCenter = Node1->NodeBounds.GetCenter();
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

float OctreeGraph::ManhattanDistance(const OctreeNode* From, const OctreeNode* To)
{
	//Standard Manhattan dist calculation.
	const FVector Point1 = From->NodeBounds.GetCenter();
	const FVector Point2 = To->NodeBounds.GetCenter();

	const float Dx = FMath::Abs(Point2.X - Point1.X);
	const float Dy = FMath::Abs(Point2.Y - Point1.Y);
	const float Dz = FMath::Abs(Point2.Z - Point1.Z);

	return Dx + Dy + Dz;
}

OctreeNode* OctreeGraph::FindGraphNode(const FVector& Location)
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
		//Node is not inside any of the roots
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

	//Because of the parent-child relationship, sometimes there will be nodes that may have child but also occupied at the same.
	//Occupied means that it intersects, but it will have children that may not intersect.
	//They will be filtered out when adding them to Nodes but cannot delete them as it would delete their children who are useful.
	if (CurrentNode != nullptr && Nodes.Contains(CurrentNode))
	{
		return CurrentNode;
	}
	
	return nullptr;
}
