// Fill out your copyright notice in the Description page of Project Settings.


#include "Pathfinding/OctreeGraph.h"

#include <queue>
#include <vector>

#include "Pathfinding/OctreeNode.h"

OctreeGraph::OctreeGraph()
{
	UE_LOG(LogTemp, Warning, TEXT("new OctreeGraph"));
	//OctreeNodes = FOctreeNodes();
}

OctreeGraph::~OctreeGraph()
{
	UE_LOG(LogTemp, Warning, TEXT("OctreeGraph destroyed"));
}


void OctreeGraph::ConnectNodes(const TSharedPtr<OctreeNode>& RootNode)
{
	TArray<TSharedPtr<OctreeNode>> NodeList;
	NodeList.Add(RootNode);
	for (int i = 0; i < NodeList.Num(); i++)
	{
		if (NodeList[i]->NavigationNode)
		{
			for (auto Direction : Directions)
			{
				const FVector NeighborLocation = NodeList[i]->NodeBounds.GetCenter() + Direction * NodeList[i]->NodeBounds.GetSize().X;
				TSharedPtr<OctreeNode> PotentialNeighbor = FindGraphNode(NeighborLocation, RootNode);

				if (PotentialNeighbor != nullptr && PotentialNeighbor->NavigationNode && !NodeList[i]->Neighbors.Contains(PotentialNeighbor)
					&& NodeList[i]->NodeBounds.Intersect(PotentialNeighbor->NodeBounds))
				{
					NodeList[i]->Neighbors.Add(PotentialNeighbor);
					NodeList[i]->NeighborCenters.Add(PotentialNeighbor->NodeBounds.GetCenter());

					PotentialNeighbor->Neighbors.Add(NodeList[i]);
					PotentialNeighbor->NeighborCenters.Add(NodeList[i]->NodeBounds.GetCenter());
				}
			}
		}

		for (auto Child : NodeList[i]->ChildrenOctreeNodes)
		{
			NodeList.Add(Child);
		}
	}
}


//Can do weighted H to increase performance. Higher numbers should yield faster path finding but might sacrifice accuracy.
static float ExtraHWeight = 3.0f;

bool OctreeGraph::OctreeAStar(const FVector& StartLocation, const FVector& EndLocation, const TSharedPtr<OctreeNode>& RootNode, TArray<FVector>& OutPathList)
{
	TSharedPtr<OctreeNode> Start = FindGraphNode(StartLocation, RootNode);
	const TSharedPtr<OctreeNode> End = FindGraphNode(EndLocation, RootNode);


	if (Start == nullptr || End == nullptr)
	{
		return false;
	}

	std::priority_queue<TSharedPtr<OctreeNode>, std::vector<TSharedPtr<OctreeNode>>, FOctreeNodeCompare> OpenSet;
	TArray<TSharedPtr<OctreeNode>> OpenList; //I use it to keep track of all the nodes used to reset them and also check which ones I checked before.
	TArray<TSharedPtr<OctreeNode>> ClosedList;
	
	Start->G = 0;
	Start->H = ManhattanDistance(Start, End) * ExtraHWeight;
	Start->F = Start->H;

	OpenSet.push(Start);
	OpenList.Add(Start);

	while (!OpenSet.empty())
	{
		TSharedPtr<OctreeNode> CurrentNodeSharedPtr = OpenSet.top();

		if (CurrentNodeSharedPtr == nullptr) return false;

		if (CurrentNodeSharedPtr == End)
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
		ClosedList.Add(CurrentNodeSharedPtr);

		if (CurrentNodeSharedPtr->Neighbors.IsEmpty()) continue;

		for (const auto NeighborShared : CurrentNodeSharedPtr->Neighbors)
		{
			if (NeighborShared == nullptr || !NeighborShared.IsValid() || ClosedList.Contains(NeighborShared)) continue;
			//We already have the best route to that node.

			TSharedPtr<OctreeNode> Neighbor = NeighborShared.Pin();

			float GWeight = 1;

			//If I will ever have time, I need to work on the whole floatable things.
		    if (Neighbor != nullptr && !Neighbor->Floatable) GWeight = 10;


			const float TentativeG = CurrentNodeSharedPtr->G + ManhattanDistance(CurrentNodeSharedPtr, Neighbor) * GWeight;

			if (Neighbor->G <= TentativeG) continue;

			Neighbor->G = TentativeG;
			Neighbor->H = ManhattanDistance(Neighbor, End) * ExtraHWeight; // Can do weighted to increase performance
			Neighbor->F = Neighbor->G + Neighbor->H;

			Neighbor->CameFrom = CurrentNodeSharedPtr;

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

void OctreeGraph::ReconstructPath(const TSharedPtr<OctreeNode>& Start, const TSharedPtr<OctreeNode>& End, TArray<FVector>& OutPathList)
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

	//I am not checking validity because it must be valid. If not, I know where to look, but it should be impossible to be invalid.
	TSharedPtr<OctreeNode> CameFrom = End->CameFrom.Pin();
	TSharedPtr<OctreeNode> Previous = End;

	while (CameFrom != nullptr && CameFrom != Start)
	{
		if (Previous->NodeBounds.GetSize().X != CameFrom->NodeBounds.GetSize().X)
		{
			const FVector BufferVector = DirectionTowardsSharedFaceFromSmallerNode(Previous, CameFrom);
			OutPathList.Insert(BufferVector, 0);
		}

		OutPathList.Insert(CameFrom->NodeBounds.GetCenter(), 0);
		Previous = CameFrom;
		CameFrom = CameFrom->CameFrom.Pin();
	}
}

FVector OctreeGraph::DirectionTowardsSharedFaceFromSmallerNode(const TSharedPtr<OctreeNode>& Node1, const TSharedPtr<OctreeNode>& Node2)
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

float OctreeGraph::ManhattanDistance(const TSharedPtr<OctreeNode>& From, const TSharedPtr<OctreeNode>& To)
{
	//Standard Manhattan dist calculation.
	const FVector Point1 = From->NodeBounds.GetCenter();
	const FVector Point2 = To->NodeBounds.GetCenter();

	const float Dx = FMath::Abs(Point2.X - Point1.X);
	const float Dy = FMath::Abs(Point2.Y - Point1.Y);
	const float Dz = FMath::Abs(Point2.Z - Point1.Z);

	return Dx + Dy + Dz;
}


TSharedPtr<OctreeNode> OctreeGraph::FindGraphNode(const FVector& Location, TSharedPtr<OctreeNode> RootNode)
{
	TSharedPtr<OctreeNode> CurrentNode = RootNode;

	if (CurrentNode == nullptr)
	{
		return nullptr;
	}

	while (CurrentNode != nullptr && !CurrentNode->ChildrenOctreeNodes.IsEmpty())
	{
		TSharedPtr<OctreeNode> Closest = CurrentNode->ChildrenOctreeNodes[0];
		for (const auto Node : CurrentNode->ChildrenOctreeNodes)
		{
			if (Node->NodeBounds.IsInside(Location))
			{
				Closest = Node;
				break;
			}
		}
		CurrentNode = Closest;
	}

	//Because of the parent-child relationship, sometimes there will be nodes that may have child but also occupied at the same.
	//Occupied means that it intersects, but it will have children that may not intersect.
	//They will be filtered out when adding them to NodeBounds but cannot delete them as it would delete their children who are useful.
	if (CurrentNode != nullptr && CurrentNode->NavigationNode)
	{
		return CurrentNode;
	}

	return nullptr;
}

void OctreeGraph::ReconstructPointersForNodes(const TSharedPtr<OctreeNode>& RootNode)
{
	TArray<TSharedPtr<OctreeNode>> NodeList;
	NodeList.Add(RootNode);

	for (int i = 0; i < NodeList.Num(); i++)
	{
		for (const auto& ChildCenter : NodeList[i]->ChildCenters)
		{
			if (TSharedPtr<OctreeNode> FoundNode = FindGraphNode(ChildCenter, RootNode); FoundNode != nullptr)
			{
				NodeList[i]->ChildrenOctreeNodes.Add(FoundNode);
			}
		}

		for (const auto& NeighborCenter : NodeList[i]->NeighborCenters)
		{
			if (TSharedPtr<OctreeNode> FoundNode = FindGraphNode(NeighborCenter, RootNode); FoundNode != nullptr)
			{
				NodeList[i]->Neighbors.Add(FoundNode);
			}
		}

		for (const auto Child : NodeList[i]->ChildrenOctreeNodes)
		{
			NodeList.Add(Child);
		}
	}
}
