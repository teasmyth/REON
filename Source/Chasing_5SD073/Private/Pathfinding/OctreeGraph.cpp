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

//This will not work in multiple pathfinding agent situations. It is a quick fix for the current situation.
TWeakPtr<OctreeNode> OctreeGraph::PreviousValidStart = nullptr;
TWeakPtr<OctreeNode> OctreeGraph::PreviousValidEnd = nullptr;


//Can do weighted H to increase performance. Higher numbers should yield faster path finding but might sacrifice accuracy.
static float ExtraHWeight = 3.0f;
static float MaxPathfindingTime = 1.0f;


bool OctreeGraph::LazyOctreeAStar(const bool& ThreadIsPaused, const bool& Debug, const TArray<FBox>& ActorBoxes, const float& MinSize,
                                  const FVector& StartLocation, const FVector& EndLocation, const TSharedPtr<OctreeNode>& RootNode,
                                  TArray<FVector>& OutPathList)
{
	const double StartTime = FPlatformTime::Seconds();


	TSharedPtr<OctreeNode> Start = RootNode->LazyDivideAndFindNode(ThreadIsPaused, ActorBoxes, MinSize, StartLocation, false);
	TSharedPtr<OctreeNode> End = RootNode->LazyDivideAndFindNode(ThreadIsPaused, ActorBoxes, MinSize, EndLocation, false);


	if (Start == nullptr || End == nullptr)
	{
		/*
		if (Start == nullptr)
		{
			Start = PreviousValidStart.Pin();
		}
		else PreviousValidStart = Start.ToWeakPtr();

		if (End == nullptr)
		{
			End = PreviousValidEnd.Pin();
		}
		else PreviousValidEnd = End.ToWeakPtr();
		*/

		if (Start == nullptr || End == nullptr)
		{
			if (Debug) UE_LOG(LogTemp, Warning, TEXT("Start or End is out of bounds."));
			return false;
		}
	}

	float PathfindingTimer = FPlatformTime::Seconds();
	Start->PathfindingData = MakeShareable(new FPathfindingNode());
	End->PathfindingData = MakeShareable(new FPathfindingNode());

	//In case we are using an occupied node as an end, we force finding neighbors here,
	//As the start will get their neighbor called anyway due to how Current Node works
	//But the End will never be neighbor of any if it is occupied.
	//Hence, forcing it here to have neighbors.
	if (End->Occupied)
	{
		if (!GetNeighbors(ThreadIsPaused, RootNode, End, ActorBoxes, MinSize))
		{
			if (Debug) UE_LOG(LogTemp, Warning, TEXT("End node is inaccessible."));
			return false; //End node is inaccessible.
		}
	}

	std::priority_queue<TSharedPtr<OctreeNode>, std::vector<TSharedPtr<OctreeNode>>, FPathfindingNodeCompare> OpenQueue;
	TSet<TSharedPtr<OctreeNode>> OpenSet; //I use it to keep track of all the nodes used to reset them and also check which ones I checked before.
	TSet<TSharedPtr<OctreeNode>> ClosedSet;

	Start->PathfindingData->G = 0;
	Start->PathfindingData->H = ManhattanDistance(Start, End) * ExtraHWeight;
	Start->PathfindingData->F = Start->PathfindingData->H;

	OpenQueue.push(Start);
	OpenSet.Add(Start);

	PathfindingMemoryTick++;

	while (!OpenQueue.empty() && !ThreadIsPaused && FPlatformTime::Seconds() - PathfindingTimer <= MaxPathfindingTime)
	{
		TSharedPtr<OctreeNode> CurrentNode = OpenQueue.top();

		if (CurrentNode == nullptr) return false;

		if (CurrentNode == End)
		{
			ReconstructPath(Start, End, OutPathList);
			OutPathList.Add(EndLocation);

			for (const auto& Node : OpenSet)
			{
				if (Node.IsValid())
				{
					Node->PathfindingData->G = FLT_MAX;
					Node->PathfindingData->H = FLT_MAX;
					Node->PathfindingData->F = FLT_MAX;
					Node->PathfindingData->CameFrom.Reset();
				}
			}

			ClosedSet.Empty();

			if (PathfindingMemoryTick > MemoryCleanupFrequency)
			{
				//Given I use root node thousands of times, making it a non const reference is not a good idea.
				//So I will just loop through its children to clean up
				//Because I might reset the pointer of the passed node, I cant pass in a const reference to CleanupUnusedNodes.
				//The reason I do it for grandchild, is because I should not reset the children of the root node.
				//Because, if we don't use auto encapsulation, we have 'custom' first children, which cannot be remade via MakeChild().
				int DeletedChildren = 0;
				for (const auto& Child : RootNode->ChildrenOctreeNodes)
				{
					for (auto& GrandChild : Child->ChildrenOctreeNodes)
					{
						if (GrandChild.IsValid()) CleanupUnusedNodes(GrandChild, OpenSet, DeletedChildren);
					}
				}
				PathfindingMemoryTick = 0;
				if (Debug) UE_LOG(LogTemp, Warning, TEXT("Octree memory cleanup. Deleted %i nodes."), DeletedChildren);
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

		CurrentNode->MemoryOptimizerTick++;
		OpenQueue.pop();
		ClosedSet.Add(CurrentNode);

		//Return false there are no neighbors. 
		if (!GetNeighbors(ThreadIsPaused, RootNode, CurrentNode, ActorBoxes, MinSize)) continue;


		for (const auto& NeighborWeakPtr : CurrentNode->PathfindingData->Neighbors)
		{
			if (ThreadIsPaused) return false;

			TSharedPtr<OctreeNode> NeighborPtr = NeighborWeakPtr.Pin();

			if (!NeighborPtr.IsValid() || ClosedSet.Contains(NeighborPtr)) continue;

			const TSharedPtr<FPathfindingNode>& NeighborData = NeighborPtr->PathfindingData;

			const float TentativeG = CurrentNode->PathfindingData->G + ManhattanDistance(CurrentNode, NeighborPtr);

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
	for (const auto& Node : OpenSet)
	{
		if (Node.IsValid())
		{
			Node->PathfindingData->G = FLT_MAX;
			Node->PathfindingData->H = FLT_MAX;
			Node->PathfindingData->F = FLT_MAX;
			Node->PathfindingData->CameFrom.Reset();
		}
	}

	if (Debug) UE_LOG(LogTemp, Error, TEXT("Couldn't find path"));
	return false;
}

bool OctreeGraph::GetNeighbors(const bool& ThreadIsPaused, const TSharedPtr<OctreeNode>& RootNode, const TSharedPtr<OctreeNode>& CurrentNode,
                               const TArray<FBox>& ActorBoxes, const float& MinSize)
{
	//Cleaning up the neighbors list from invalid pointers.
	TSet<TWeakPtr<OctreeNode>>& Neighbors = CurrentNode->PathfindingData->Neighbors;

	if (Neighbors.Contains(nullptr))
	{
		TSet<TWeakPtr<OctreeNode>> ValidNeighbors;
		for (const auto& Neighbor : Neighbors)
		{
			//!Occupied is explained at the bottom of LazyDivideAndFindNode() in OctreeNode.cpp.
			if (Neighbor.IsValid() && !Neighbor.Pin()->Occupied)
			{
				ValidNeighbors.Add(Neighbor);
			}
		}
		Neighbors = ValidNeighbors;
	}

	int NeighborCount = 0;
	TArray<TArray<FVector>> PotentialNeighborPositions;
	for (int i = 0; i < 6; i++)
	{
		PotentialNeighborPositions.Add(CalculatePositions(CurrentNode, i, MinSize));
		NeighborCount += PotentialNeighborPositions[i].Num();
	}

	//If we already have all the neighbors, no need to continue. Else, we still made useful calculation for the neighbors.
	if (Neighbors.Num() == NeighborCount)
	{
		return true;
	}

	for (int i = 0; i < 6; i++)
	{
		for (const auto& Pos : PotentialNeighborPositions[i])
		{
			if (ThreadIsPaused) return false;

			const TSharedPtr<OctreeNode> Node = RootNode->LazyDivideAndFindNode(ThreadIsPaused, ActorBoxes, MinSize, Pos, true);
			if (Node.IsValid() && !Neighbors.Contains(Node.ToWeakPtr()))
			{
				Neighbors.Add(Node.ToWeakPtr());
				if (!Node->PathfindingData.IsValid())
				{
					Node->PathfindingData = MakeShareable(new FPathfindingNode());
				}
				Node->PathfindingData->Neighbors.Add(CurrentNode.ToWeakPtr());
			}
		}
	}

	return !Neighbors.IsEmpty();
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

TArray<FVector> OctreeGraph::CalculatePositions(const TSharedPtr<OctreeNode>& CurrentNode, const int& Face, const float& MinNodeSize)
{
	TArray<FVector> PotentialNeighborPositions;

	// Calculate the start position of the face
	const FVector StartPosition = CurrentNode->Position + FVector(DIRECTIONS[Face]) * CurrentNode->HalfSize * 1.01f;

	// Calculate the number of steps in each direction
	const int Steps = FMath::RoundToInt(CurrentNode->HalfSize * 2.0f / MinNodeSize);
	//Because if it is 1.9999 due to float error, it would be rounded to 1 by default.

	FVector Axis1, Axis2;
	if (DIRECTIONS[Face] == FIntVector(1, 0, 0) || DIRECTIONS[Face] == FIntVector(-1, 0, 0))
	{
		Axis1 = FVector(0, 1, 0);
		Axis2 = FVector(0, 0, 1);
	}
	else if (DIRECTIONS[Face] == FIntVector(0, 1, 0) || DIRECTIONS[Face] == FIntVector(0, -1, 0))
	{
		Axis1 = FVector(1, 0, 0);
		Axis2 = FVector(0, 0, 1);
	}
	else //if (DIRECTIONS[Face].Equals(FVector(0, 0, 1)) || DIRECTIONS[Face].Equals(FVector(0, 0, -1)))
	{
		Axis1 = FVector(1, 0, 0);
		Axis2 = FVector(0, 1, 0);
	}

	if (Steps == 1) //Meaning it is a minimum-sized node.
	{
		PotentialNeighborPositions.Add(StartPosition);
		return PotentialNeighborPositions;
	}

	// Iterate over the face in steps of the size of the minimum node
	for (int i = -Steps / 2; i <= Steps / 2; i++) //Because of the octree structure, steps will always be a multiple of 2. Int is safe.
	{
		if (i == 0) continue;

		for (int j = -Steps / 2; j <= Steps / 2; j++)
		{
			if (j == 0) continue;

			// Calculate the position
			const FVector PotentialNeighborPosition = StartPosition + Axis1 * i * MinNodeSize + Axis2 * j * MinNodeSize;

			// Add the position to the list
			PotentialNeighborPositions.Add(PotentialNeighborPosition);
		}
	}

	return PotentialNeighborPositions;
}

void OctreeGraph::CleanupUnusedNodes(TSharedPtr<OctreeNode>& Node, const TSet<TSharedPtr<OctreeNode>>& OpenSet, int& DeletedChildrenCount)
{
	for (auto& Child : Node->ChildrenOctreeNodes)
	{
		if (!Child.IsValid()) continue;

		if (!Child->ChildrenOctreeNodes.IsEmpty())
		{
			CleanupUnusedNodes(Child, OpenSet, DeletedChildrenCount);
			continue;
		}


		//We want to keep the nodes that were just used in case they were just created.
		if (Child->MemoryOptimizerTick < MemoryOptimizerTickThreshold && !OpenSet.Contains(Child))
		{
			Child.Reset();
			DeletedChildrenCount++;
		}
		else
		{
			Child->MemoryOptimizerTick = 0;
			Node->NodeIsInUse = true;
			if (Child->PathfindingData.IsValid())
			{
				for (auto& Neighbor : Child->PathfindingData->Neighbors)
				{
					if (Neighbor.IsValid() && !Neighbor.Pin()->Occupied) continue;
					Neighbor.Reset(); //Resetting the neighbor if it is occupied.
				}
			}
		}
	}

	if (!Node->NodeIsInUse) Node.Reset();
}
