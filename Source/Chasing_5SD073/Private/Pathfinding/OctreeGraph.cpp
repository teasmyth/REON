// Fill out your copyright notice in the Description page of Project Settings.


#include "Pathfinding/OctreeGraph.h"

#include <queue>
#include <vector>

#include "Pathfinding/OctreeNode.h"

OctreeGraph::OctreeGraph()
{
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
			if (ClosedList.Contains(Neighbor) || Neighbor == nullptr) continue; //We already have the best route to that node.
			
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

	//First I check whether the location is inside any of the root nodes
	OctreeNode* CurrentNode = nullptr;

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
		UE_LOG(LogTemp, Warning, TEXT("Couldn't find node."));		
		return nullptr;
	}

	//Then I check to which children it is closer. Since they are divided evenly, it will always be closer to the needed node.
	while (!CurrentNode->ChildrenOctreeNodes.IsEmpty())
	{
		OctreeNode* Closest = CurrentNode->ChildrenOctreeNodes[0];
		for (const auto Node : CurrentNode->ChildrenOctreeNodes)
		{
			if(FVector::DistSquared(Closest->NodeBounds.GetCenter(), Location) >= FVector::DistSquared(Node->NodeBounds.GetCenter(), Location))
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
