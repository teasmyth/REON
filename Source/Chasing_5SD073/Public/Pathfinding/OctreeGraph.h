// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OctreeNode.h"

class CHASING_5SD073_API OctreeGraph
{
public:
	OctreeGraph();
	~OctreeGraph();
		
	static void ConnectNodes(const bool& Loading, const TSharedPtr<OctreeNode>& RootNode, const TSharedPtr<OctreeNode>& Node);
	static bool OctreeAStar(const bool& Debug, FLargeMemoryReader& OctreeData, const FVector& StartLocation, const FVector& EndLocation, const TSharedPtr<OctreeNode>& RootNode, TArray<FVector>& OutPathList);

	static TSharedPtr<OctreeNode> FindGraphNode(const FVector& Location, const TSharedPtr<OctreeNode>& RootNode);
	static TSharedPtr<OctreeNode> FindAndLoadNode(FLargeMemoryReader& OctreeData, const FVector& Location, const TSharedPtr<OctreeNode>& RootNode);
	static bool GetNeighbors(FLargeMemoryReader& OctreeData, const TSharedPtr<OctreeNode>& Node, const TSharedPtr<OctreeNode>& RootNode);
	
	static FVector DirectionTowardsSharedFaceFromSmallerNode(const TSharedPtr<OctreeNode>& Node1, const TSharedPtr<OctreeNode>& Node2);
	static float ManhattanDistance(const TSharedPtr<OctreeNode>& From, const TSharedPtr<OctreeNode>& To);
	static void ReconstructPath(const TSharedPtr<OctreeNode>& Start, const TSharedPtr<OctreeNode>& End, TArray<FVector>& OutPathList);
	
	static TArray<int64> RootNodeIndexData;
	static TArray<double> TimeTaken;

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

struct FPathfindingNodeCompare
{
	//Will put the lowest FScore above all
	bool operator()(const TSharedPtr<OctreeNode>& Node1, const TSharedPtr<OctreeNode>& Node2) const
	{
		return (Node1->PathfindingData->F > Node2->PathfindingData->F);
	}
};
