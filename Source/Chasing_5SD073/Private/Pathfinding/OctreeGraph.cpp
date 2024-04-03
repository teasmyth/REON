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

void OctreeGraph::ConnectNodes(const bool& Loading, const TSharedPtr<OctreeNode>& RootNode)
{
	TArray<TSharedPtr<OctreeNode>> NodeList;
	NodeList.Add(RootNode);
	int Count = 0;

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
				if (PotentialNeighbor != nullptr /* && OctreeNode::DoNodesIntersect(NodeList[i], PotentialNeighbor)*/ && !NodeList[i]->Neighbors.Contains(PotentialNeighbor))
				{
					Count++;
					NodeList[i]->Neighbors.Add(PotentialNeighbor);
					PotentialNeighbor->Neighbors.Add(NodeList[i]);
				}
			}
		}

		for (const auto& Child : NodeList[i]->ChildrenOctreeNodes)
		{
			NodeList.Add(Child);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Total Connections: %d"), Count);
}


//Can do weighted H to increase performance. Higher numbers should yield faster path finding but might sacrifice accuracy.
static float ExtraHWeight = 3.0f;

bool OctreeGraph::OctreeAStar(const bool& Debug, const FVector& StartLocation, const FVector& EndLocation, const TSharedPtr<OctreeNode>& RootNode,
                              TArray<FVector>& OutPathList)
{
	const TSharedPtr<OctreeNode> Start = FindGraphNode(StartLocation, RootNode); //check root node and make sure child parent relationship is correct.
	const TSharedPtr<OctreeNode> End = FindGraphNode(EndLocation, RootNode);

	if (Start == nullptr || End == nullptr)
	{
		if (Debug) UE_LOG(LogTemp, Warning, TEXT("Start or End is nullptr."));
		return false;
	}

	const double StartTime = FPlatformTime::Seconds();

	Start->PathfindingData = MakeShareable(new FPathfindingNode(Start));
	End->PathfindingData = MakeShareable(new FPathfindingNode(End));

	//std::priority_queue<TSharedPtr<OctreeNode>, std::vector<TSharedPtr<OctreeNode>>, FOctreeNodeCompare> OpenSet;
	std::priority_queue<TSharedPtr<OctreeNode>, std::vector<TSharedPtr<OctreeNode>>, FPathfindingNodeCompare> TestOpenSet;


	TSet<TSharedPtr<OctreeNode>> OpenList; //I use it to keep track of all the nodes used to reset them and also check which ones I checked before.
	TSet<TSharedPtr<OctreeNode>> ClosedList;

	//Start->G = 0;
	//Start->H = ManhattanDistance(Start, End) * ExtraHWeight;
	//Start->F = Start->H;

	Start->PathfindingData->G = 0;
	Start->PathfindingData->H = ManhattanDistance(Start, End) * ExtraHWeight;
	Start->PathfindingData->F = Start->PathfindingData->H;

	//OpenSet.push(Start);

	TestOpenSet.push(Start);
	OpenList.Add(Start);


	/*
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

			TimeTaken.Add( FPlatformTime::Seconds() - StartTime);

			float Total = 0;
			for (const auto Time : TimeTaken)
			{
				Total += Time;
			}

			UE_LOG(LogTemp, Warning, TEXT("Path found in avg. in %f seconds"), Total / (float)TimeTaken.Num());
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
			//if (Neighbor != nullptr && !Neighbor->Floatable) GWeight = 10;


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
	*/


	while (!TestOpenSet.empty())
	{
		TSharedPtr<OctreeNode> CurrentNode = TestOpenSet.top();

		if (CurrentNode == nullptr) return false;

		if (CurrentNode == End)
		{
			TestReconstructPath(Start, End, OutPathList);
			OutPathList.Add(EndLocation);

			for (const auto Node : OpenList)
			{
				Node->PathfindingData.Reset();
			}

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

		TestOpenSet.pop();
		ClosedList.Add(CurrentNode);

		//if (!GetNeighbors(CurrentNode, RootNode)) continue;

		if (CurrentNode->Neighbors.IsEmpty()) continue;

		for (const auto& NeighborWeakPtr : CurrentNode->Neighbors)
		{
			TSharedPtr<OctreeNode> NeighborPtr = NeighborWeakPtr.Pin();

			if (!NeighborPtr.IsValid() || ClosedList.Contains(NeighborPtr)) continue;

			TSharedPtr<FPathfindingNode>& NeighborData = NeighborPtr->PathfindingData;

			if (!NeighborData.IsValid())
			{
				NeighborData = MakeShareable(new FPathfindingNode(NeighborPtr));
			}

			float GWeight = 1;

			if (NeighborPtr->Floatable)
			{
				//GWeight = 10;
			}

			const float TentativeG = CurrentNode->PathfindingData->G + ManhattanDistance(CurrentNode, NeighborPtr) * GWeight;

			if (NeighborData->G <= TentativeG) continue;

			NeighborData->G = TentativeG;
			NeighborData->H = ManhattanDistance(NeighborPtr, End) * ExtraHWeight; // Can do weighted to increase performance
			NeighborData->F = NeighborData->G + NeighborData->H;

			NeighborData->CameFrom = CurrentNode.ToWeakPtr();

			if (!OpenList.Contains(NeighborPtr))
			{
				OpenList.Add(NeighborPtr);
				TestOpenSet.push(NeighborPtr);
			}
		}
	}

	if (Debug) UE_LOG(LogTemp, Error, TEXT("Couldn't find path"));
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

	/*
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
	*/
}

void OctreeGraph::TestReconstructPath(const TSharedPtr<OctreeNode>& Start, const TSharedPtr<OctreeNode>& End,
                                      TArray<FVector>& OutPathList)
{
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

bool OctreeGraph::GetNeighbors(const TSharedPtr<OctreeNode>& Node, const TSharedPtr<OctreeNode>& RootNode)
{
	if (!Node->Neighbors.IsEmpty())
	{
		return true;
	}
	const FVector Center = Node->Position;
	const float HalfSize = Node->HalfSize * 1.01f; //little bit of padding to touch neighboring node.

	for (const auto& Direction : Directions)
	{
		const FVector NeighborLocation = Center + Direction * HalfSize;
		const TSharedPtr<OctreeNode> PotentialNeighbor = FindGraphNode(NeighborLocation, RootNode);
		//This already checks whether it is occupied or not.

		//Physically speaking, any node just outside the border of the current node is a neighbor. Not using DoNodeIntersect sacrifices minimal amount of accuracy.
		//In exchange for faster neighbor building.
		if (PotentialNeighbor != nullptr && OctreeNode::DoNodesIntersect(Node, PotentialNeighbor) && !Node->Neighbors.Contains(PotentialNeighbor))
		{
			Node->Neighbors.Add(PotentialNeighbor);
			PotentialNeighbor->Neighbors.Add(Node);
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
	//const FVector Point1 = From->NodeBounds.GetCenter();
	const FVector Point1 = From->Position;
	//const FVector Point2 = To->NodeBounds.GetCenter();
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
			//if (Node->NodeBounds.IsInside(Location))
			if (Node.IsValid() && Node->IsInsideNode(Location))
			{
				CurrentNode = Node;
				FoundNode = true;
				break;
			}

			FoundNode = false;
		}
	}

	/*
	const TSharedPtr<OctreeNode> TestNode = CurrentNode;
	UE_LOG(LogTemp, Warning, TEXT("Found Node: %s"), TestNode.IsValid() ? TEXT("True") : TEXT("False"));
*/
	//Because of the parent-child relationship, sometimes there will be nodes that may have child but also occupied at the same.
	//Occupied means that it intersects, but it will have children that may not intersect.
	//They will be filtered out when adding them to NodeBounds but cannot delete them as it would delete their children who are useful.
	if (CurrentNode.IsValid() && !CurrentNode->Occupied && FoundNode)
	{
		return CurrentNode;
	}

	return nullptr;
}


//Deprecated. Don't use this.
void OctreeGraph::ReconstructPointersForNodes(const TSharedPtr<OctreeNode>& RootNode)
{
	TArray<TSharedPtr<OctreeNode>> NodeList;
	NodeList.Add(RootNode);

	for (int i = 0; i < NodeList.Num(); i++)
	{
		/*
		for (const auto& NeighborCenter : NodeList[i]->NeighborCenters)
		{
			if (TSharedPtr<OctreeNode> FoundNode = FindGraphNode(NeighborCenter, RootNode); FoundNode != nullptr)
			{
				NodeList[i]->Neighbors.Add(FoundNode);
			}
		}
		*/

		for (const auto Child : NodeList[i]->ChildrenOctreeNodes)
		{
			NodeList.Add(Child);
		}
	}
}
