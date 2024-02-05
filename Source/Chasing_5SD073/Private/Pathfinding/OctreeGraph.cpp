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
	OctreeGraphNode* End = FindGraphNode(EndLocation);


	if (Start == nullptr || End == nullptr)
	{
		return false;
	}

	OutPathList.Empty();
	TArray<OctreeGraphNode*> OpenList;
	std::priority_queue<OctreeGraphNode*, std::vector<OctreeGraphNode*>, FOctreeGraphNodeCompare> OpenSet;
	TArray<OctreeGraphNode*> ClosedList;

	Start->G = 0;
	Start->H = FVector::DistSquared(Start->Bounds.GetCenter(), End->Bounds.GetCenter());
	Start->F = Start->H;

	OpenList.Add(Start);
	OpenSet.push(Start);

	while (!OpenSet.empty())
	{
		OctreeGraphNode* CurrentNode = OpenSet.top();
		OpenSet.pop();
		OpenList.Remove(CurrentNode);

		ClosedList.Add(CurrentNode);

		if (CurrentNode == End)
		{
			ReconstructPath(Start, End, OutPathList);
			UE_LOG(LogTemp, Warning, TEXT("Path length: %i"), OutPathList.Num());


			for (const auto Node : OpenList)
			{
				Node->G = FLT_MAX;
				Node->F = FLT_MAX;
				Node->H = FLT_MAX;
				Node->CameFrom = nullptr;
			}
			return true;
		}

		if (CurrentNode->Neighbors.IsEmpty()) continue;
		
		for (const auto Neighbor : CurrentNode->Neighbors)
		{
			if (ClosedList.Contains(Neighbor)) continue; //We already have the best route to that node.
			
			const float TentativeG = CurrentNode->G + FVector::DistSquared(CurrentNode->Bounds.GetCenter(), Neighbor->Bounds.GetCenter());

			if (Neighbor->G <= TentativeG) continue;

			Neighbor->G = TentativeG;
			Neighbor->H = FVector::DistSquared(Neighbor->Bounds.GetCenter(), End->Bounds.GetCenter());
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

	if (Start == End)
	{
		return;
	}
	
	OctreeGraphNode* CameFrom = End->CameFrom;
	while (CameFrom != nullptr && CameFrom != Start)
	{
		OutPathList.Add(CameFrom->Bounds.GetCenter());
		CameFrom = CameFrom->CameFrom;
	}

	OutPathList.Add(Start->Bounds.GetCenter());
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
