// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OctreeNode.h"

//TODO remove all the values present here from OctreeNode, and just make a new FPathfindingData when needed.
struct FPathfindingData
{
	TArray<OctreeNode*> Nodes;
	TArray<float> F;
	TArray<float> G;
	TArray<float> H;
	TArray<OctreeNode*> CameFrom;

	void AddNode(OctreeNode* NodeToAdd)
	{
		Nodes.Add(NodeToAdd);
		F.Add(FLT_MAX);
		G.Add(FLT_MAX);
		H.Add(FLT_MAX);
		CameFrom.Add(nullptr);
	}

	void RemoveNode(OctreeNode* NodeToRemove)
	{
		const int Index = Nodes.Find(NodeToRemove);
		if (Index != INDEX_NONE)
		{
			Nodes.RemoveAt(Index);
			F.RemoveAt(Index);
			G.RemoveAt(Index);
			H.RemoveAt(Index);
			CameFrom.RemoveAt(Index);
		}
	}

	int GetLowestIndex() const
	{
		int Index = -1;
		float LowestF = FLT_MAX;
		for (int i = 0; i < F.Num(); i++)
		{
			if (F[i] < LowestF)
			{
				LowestF = F[i];
				Index = i;
			}
		}
		return Index; 
	}
};


class CHASING_5SD073_API OctreeGraph
{
public:
	OctreeGraph();
	~OctreeGraph();
		
	static void ConnectNodes(const TSharedPtr<OctreeNode>& RootNode);
	static bool OctreeAStar(const FVector& StartLocation, const FVector& EndLocation, TSharedPtr<OctreeNode> RootNode, TArray<FVector>& OutPathList);
	static void ReconstructPath(const TSharedPtr<OctreeNode>& Start, const TSharedPtr<OctreeNode>& End, TArray<FVector>& OutPathList);
	static FVector DirectionTowardsSharedFaceFromSmallerNode(const TSharedPtr<OctreeNode>& Node1, const TSharedPtr<OctreeNode>& Node2);
	static float ManhattanDistance(const TSharedPtr<OctreeNode>& From, const TSharedPtr<OctreeNode>& To);
	static TSharedPtr<OctreeNode> FindGraphNode(const FVector& Location, TSharedPtr<OctreeNode> RootNode);
	static void ReconstructPointersForNodes(const TSharedPtr<OctreeNode>& RootNode);

private:
	inline static TArray<FVector> Directions
	{
		FVector(1, 0, 0),
		FVector(-1, 0, 0),
		FVector(0, 1, 0),
		FVector(0, -1, 0),
		FVector(0, 0, 1),
		FVector(0, 0, -1)
	};
};


struct FOctreeNodeCompare
{
	//Will put the lowest FScore above all
	bool operator()(const TSharedPtr<OctreeNode>& Node1, const TSharedPtr<OctreeNode>& Node2) const
	{
		return (Node1->F > Node2->F);
	}
};
