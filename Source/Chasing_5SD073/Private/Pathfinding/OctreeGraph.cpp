// Fill out your copyright notice in the Description page of Project Settings.


#include "Pathfinding/OctreeGraph.h"

#include <queue>
#include <vector>
#include "Pathfinding/OctreeNode.h"

OctreeGraph::OctreeGraph()
{
	UE_LOG(LogTemp, Warning, TEXT("new OctreeGraph"));
}

OctreeGraph::~OctreeGraph()
{
	UE_LOG(LogTemp, Warning, TEXT("OctreeGraph destroyed"));
}

TArray<double> OctreeGraph::TimeTaken;
TArray<int64> OctreeGraph::RootNodeIndexData = {};
TArray<int> OctreeGraph::PathLength = {};
int OctreeGraph::AvgPathLength = INT_MAX;


//Can do weighted H to increase performance. Higher numbers should yield faster path finding but might sacrifice accuracy.
static float ExtraHWeight = 3.0f;

bool OctreeGraph::OctreeAStar(const bool& Debug, FLargeMemoryReader& OctreeData, const FVector& StartLocation, const FVector& EndLocation,
                              const TSharedPtr<OctreeNode>& RootNode,
                              TArray<FVector>& OutPathList)
{
	const TSharedPtr<OctreeNode> Start = FindAndLoadNode(OctreeData, StartLocation, RootNode);
	//check root node and make sure child parent relationship is correct.
	const TSharedPtr<OctreeNode> End = FindAndLoadNode(OctreeData, EndLocation, RootNode);

	if (Start == nullptr || End == nullptr)
	{
		if (Debug) UE_LOG(LogTemp, Warning, TEXT("Start or End is nullptr."));
		return false;
	}

	const double StartTime = FPlatformTime::Seconds();

	Start->PathfindingData = MakeShareable(new FPathfindingNode(Start));
	End->PathfindingData = MakeShareable(new FPathfindingNode(End));

	std::priority_queue<TSharedPtr<OctreeNode>, std::vector<TSharedPtr<OctreeNode>>, FPathfindingNodeCompare> OpenQueue;

	TSet<TSharedPtr<OctreeNode>> OpenSet; //I use it to keep track of all the nodes used to reset them and also check which ones I checked before.
	TSet<TSharedPtr<OctreeNode>> ClosedSet;

	Start->PathfindingData->G = 0;
	Start->PathfindingData->H = ManhattanDistance(Start, End) * ExtraHWeight;
	Start->PathfindingData->F = Start->PathfindingData->H;

	OpenQueue.push(Start);
	OpenSet.Add(Start);

	while (!OpenQueue.empty() && OpenQueue.size() < AvgPathLength * 50)
	{
		TSharedPtr<OctreeNode> CurrentNode = OpenQueue.top();

		if (CurrentNode == nullptr) return false;

		if (CurrentNode == End)
		{
			ReconstructPath(Start, End, OutPathList);
			OutPathList.Add(EndLocation);

			for (const auto Node : OpenSet)
			{
				Node->PathfindingData.Reset();
			}

			PathLength.Add(OpenQueue.size());
			AvgPathLength = 0;
			for (const auto Length : PathLength)
			{
				AvgPathLength += Length;
			}
			AvgPathLength /= PathLength.Num();

			if (Debug)
			{
				TimeTaken.Add(FPlatformTime::Seconds() - StartTime);

				float Total = 0;
				for (const auto Time : TimeTaken)
				{
					Total += Time;
				}

				UE_LOG(LogTemp, Warning, TEXT("Path found in avg. in %f seconds"), Total / (float)TimeTaken.Num());
			}
			return true;
		}

		OpenQueue.pop();
		ClosedSet.Add(CurrentNode);

		if (!GetNeighbors(OctreeData, CurrentNode, RootNode)) continue;

		//if (CurrentNode->Neighbors.IsEmpty()) continue;

		for (const auto& NeighborWeakPtr : CurrentNode->Neighbors)
		{
			TSharedPtr<OctreeNode> NeighborPtr = NeighborWeakPtr.Pin();

			if (!NeighborPtr.IsValid() || ClosedSet.Contains(NeighborPtr)) continue;

			TSharedPtr<FPathfindingNode>& NeighborData = NeighborPtr->PathfindingData;

			if (!NeighborData.IsValid())
			{
				NeighborData = MakeShareable(new FPathfindingNode(NeighborPtr));
			}

			float GWeight = 1;

			if (NeighborPtr->Floatable)
			{
				GWeight = 10;
			}

			const float TentativeG = CurrentNode->PathfindingData->G + ManhattanDistance(CurrentNode, NeighborPtr) * GWeight;

			if (NeighborData->G <= TentativeG) continue;

			NeighborData->G = TentativeG;
			NeighborData->H = ManhattanDistance(NeighborPtr, End) * ExtraHWeight; // Can do weighted to increase performance
			NeighborData->F = NeighborData->G + NeighborData->H;

			NeighborData->CameFrom = CurrentNode.ToWeakPtr();

			if (!OpenSet.Contains(NeighborPtr))
			{
				OpenSet.Add(NeighborPtr);
				OpenQueue.push(NeighborPtr);
			}
		}
	}
	for (const auto Node : OpenSet)
	{
		Node->PathfindingData.Reset();
	}

	//if (Debug)
	UE_LOG(LogTemp, Error, TEXT("Couldn't find path"));
	UE_LOG(LogTemp, Error, TEXT("Couldn't find path"));
	return false;
}

void OctreeGraph::ReconstructPath(const TSharedPtr<OctreeNode>& Start, const TSharedPtr<OctreeNode>& End,
                                  TArray<FVector>& OutPathList)
{
	/* Because I am using Center position as the target in pathfinding, adding those to the list might not ensure a smooth path.
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
	TSharedPtr<OctreeNode> CameFrom = End->PathfindingData->CameFrom.Pin();
	TSharedPtr<OctreeNode> Previous = End;

	while (CameFrom != nullptr && CameFrom != Start)
	{
		if (Previous->HalfSize != CameFrom->HalfSize)
		{
			const FVector BufferVector = DirectionTowardsSharedFaceFromSmallerNode(Previous, CameFrom);
			OutPathList.Insert(BufferVector, 0);
		}

		OutPathList.Insert(CameFrom->Position, 0);
		Previous = CameFrom;
		CameFrom = CameFrom->PathfindingData->CameFrom.Pin();
	}
}

void OctreeGraph::ConnectNodes(const bool& Loading, const TSharedPtr<OctreeNode>& RootNode, const TSharedPtr<OctreeNode>& Node)
{
	/*
	TArray<TSharedPtr<OctreeNode>> NodeList;
	NodeList.Add(RootNode);

	for (int i = 0; i < NodeList.Num(); i++)
	{
		if (!Loading) break;

		if (!NodeList[i]->Occupied)
		{
			const FVector Center = NodeList[i]->Position;
			const float HalfSize = NodeList[i]->HalfSize * 1.01f; //little bit of padding to touch neighboring node.

			for (const auto& Direction : Directions)
			{
				const FVector NeighborLocation = Center + Direction * HalfSize;
				const TSharedPtr<OctreeNode> PotentialNeighbor = FindGraphNode(NeighborLocation, RootNode);
				//This already checks whether it is occupied or not.

				//Physically speaking, any node just outside the border of the current node is a neighbor. Not using DoNodeIntersect sacrifices minimal amount of accuracy.
				//In exchange for faster neighbor building.
				if (PotentialNeighbor != nullptr  && OctreeNode::DoNodesIntersect(NodeList[i], PotentialNeighbor) && !NodeList[i]->Neighbors.
					Contains(PotentialNeighbor))
				{
					NodeList[i]->Neighbors.Add(PotentialNeighbor);
					NodeList[i]->NeighborPositions.Add(PotentialNeighbor->Position);
					PotentialNeighbor->Neighbors.Add(NodeList[i]);
					PotentialNeighbor->NeighborPositions.Add(Center);
				}
			}
		}

		for (const auto& Child : NodeList[i]->ChildrenOctreeNodes)
		{
			NodeList.Add(Child);
		}
	}
*/

	if (!Loading) return;

	if (!Node->Occupied)
	{
		const FVector Center = Node->Position;
		const float HalfSize = Node->HalfSize * 1.01f; //little bit of padding to touch neighboring node.
		for (const auto& Direction : Directions)
		{
			const TSharedPtr<OctreeNode> PotentialNeighbor = FindGraphNode(Center + Direction * HalfSize, RootNode);
			if (PotentialNeighbor.IsValid() && !Node->Neighbors.Contains(PotentialNeighbor))
			{
				Node->Neighbors.Add(PotentialNeighbor.ToWeakPtr());
				Node->NeighborPositions.Add(PotentialNeighbor->Position);
				PotentialNeighbor->Neighbors.Add(Node.ToWeakPtr());
				PotentialNeighbor->NeighborPositions.Add(Center);
			}
		}
	}

	for (const auto& Child : Node->ChildrenOctreeNodes)
	{
		ConnectNodes(Loading, RootNode, Child);
	}
}

bool OctreeGraph::GetNeighbors(FLargeMemoryReader& OctreeData, const TSharedPtr<OctreeNode>& Node, const TSharedPtr<OctreeNode>& RootNode)
{
	if (Node->Neighbors.Num() == Node->NeighborPositions.Num())
	{
		return true;
	}

	for (const auto& NeighborCenter : Node->NeighborPositions)
	{
		const TWeakPtr<OctreeNode> PotentialNeighbor = FindAndLoadNode(OctreeData, NeighborCenter, RootNode).ToWeakPtr();
		if (PotentialNeighbor.IsValid() && !Node->Neighbors.Contains(PotentialNeighbor))
		{
			Node->Neighbors.Add(PotentialNeighbor);
			PotentialNeighbor.Pin()->Neighbors.Add(Node.ToWeakPtr());
		}
	}

	return !Node->Neighbors.IsEmpty();
}

FVector OctreeGraph::DirectionTowardsSharedFaceFromSmallerNode(const TSharedPtr<OctreeNode>& Node1, const TSharedPtr<OctreeNode>& Node2)
{
	float SmallSize = 1;
	FVector SmallerCenter;
	FVector LargerCenter;

	if (Node1->HalfSize < Node2->HalfSize)
	{
		SmallSize = Node1->HalfSize;
		SmallerCenter = Node1->Position;
		LargerCenter = Node2->Position;
	}
	else if (Node2->HalfSize < Node1->HalfSize)
	{
		SmallSize = Node2->HalfSize;
		SmallerCenter = Node2->Position;
		LargerCenter = Node1->Position;
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

	return SmallerCenter + Direction * SmallSize;
}


float OctreeGraph::ManhattanDistance(const TSharedPtr<OctreeNode>& From, const TSharedPtr<OctreeNode>& To)
{
	//Standard Manhattan dist calculation.
	const FVector Point1 = From->Position;
	const FVector Point2 = To->Position;

	const float Dx = FMath::Abs(Point2.X - Point1.X);
	const float Dy = FMath::Abs(Point2.Y - Point1.Y);
	const float Dz = FMath::Abs(Point2.Z - Point1.Z);

	return Dx + Dy + Dz;
}


TSharedPtr<OctreeNode> OctreeGraph::FindGraphNode(const FVector& Location, const TSharedPtr<OctreeNode>& RootNode)
{
	TSharedPtr<OctreeNode> CurrentNode = RootNode;

	if (!CurrentNode.IsValid())
	{
		return nullptr;
	}

	bool FoundNode = true;

	//If the children empty triggers first than found node, then we actually found the node.
	while (CurrentNode.IsValid() && !CurrentNode->ChildrenOctreeNodes.IsEmpty() && FoundNode)
	{
		for (const auto& Node : CurrentNode->ChildrenOctreeNodes)
		{
			if (Node.IsValid() && Node->IsInsideNode(Location))
			{
				CurrentNode = Node;
				FoundNode = true;
				break;
			}

			FoundNode = false;
		}
	}

	//Because of the parent-child relationship, sometimes there will be nodes that may have child but also occupied at the same.
	//Occupied means that it intersects, but it will have children that may not intersect.
	//They will be filtered out when adding them to NodeBounds but cannot delete them as it would delete their children who are useful.
	if (CurrentNode.IsValid() && !CurrentNode->Occupied && FoundNode)
	{
		return CurrentNode;
	}

	return nullptr;
}


TSharedPtr<OctreeNode> OctreeGraph::FindAndLoadNode(FLargeMemoryReader& OctreeData, const FVector& Location, const TSharedPtr<OctreeNode>& RootNode)
{
	TSharedPtr<OctreeNode> CurrentNode = RootNode;
	TArray<int64> ChildrenIndices = RootNodeIndexData;

	bool FoundNode = true;

	//TODO handle the case where the node is not found. Delete unused stuff. Tracker id.
	//add a new variable, tick, add one tick to each node as i traverse down, every 60 ticks, delete nodes that are under a minimum like 20
	//of course do it in a different function after every pathfinding is done.
	//no need for tracking id, just delete the nodes that are not used.
	while (CurrentNode.IsValid() && !CurrentNode->ChildrenOctreeNodes.IsEmpty() && FoundNode)
	{
		for (int i = 0; i < CurrentNode->ChildrenOctreeNodes.Num(); i++)
		{
			if (!CurrentNode->ChildrenOctreeNodes[i].IsValid())
			{
				CurrentNode->ChildrenOctreeNodes[i] = OctreeNode::LoadSingleNode(OctreeData, ChildrenIndices[i]);
			}

			if (CurrentNode->ChildrenOctreeNodes[i]->IsInsideNode(Location))
			{
				ChildrenIndices = CurrentNode->ChildrenIndexes;
				CurrentNode = CurrentNode->ChildrenOctreeNodes[i];
				FoundNode = true;
				break;
			}

			//Resetting could be waste of resource, let them chill for the time being.
			//CurrentNode->ChildrenOctreeNodes[i].Reset();
			FoundNode = false;
		}
	}

	if (CurrentNode.IsValid() && !CurrentNode->Occupied && FoundNode)
	{
		return CurrentNode;
	}

	return nullptr;
}
