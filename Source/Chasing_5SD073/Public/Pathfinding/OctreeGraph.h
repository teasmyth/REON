// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OctreeNode.h"

class CHASING_5SD073_API OctreeGraph
{
public:
	OctreeGraph();
	~OctreeGraph();
		
	static void ReconstructPointersForNodes(const TSharedPtr<OctreeNode>& RootNode);
	static void ConnectNodes(const TSharedPtr<OctreeNode>& RootNode);
	static bool OctreeAStar(const bool& Debug, const FVector& StartLocation, const FVector& EndLocation, const TSharedPtr<OctreeNode>& RootNode, TArray<FVector>& OutPathList);
	static TSharedPtr<OctreeNode> FindGraphNode(const FVector& Location, const TSharedPtr<OctreeNode>& RootNode);
	
	static void ReconstructPath(const TSharedPtr<OctreeNode>& Start, const TSharedPtr<OctreeNode>& End, TArray<FVector>& OutPathList);
	static FVector DirectionTowardsSharedFaceFromSmallerNode(const TSharedPtr<OctreeNode>& Node1, const TSharedPtr<OctreeNode>& Node2);
	static float ManhattanDistance(const TSharedPtr<OctreeNode>& From, const TSharedPtr<OctreeNode>& To);

	static void TestReconstructPath(const TSharedPtr<OctreeNode>& Start, const TSharedPtr<OctreeNode>& End, TArray<FVector>& OutPathList);

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

/*
struct FOctreeNodeCompare
{
	//Will put the lowest FScore above all
	bool operator()(const TSharedPtr<OctreeNode>& Node1, const TSharedPtr<OctreeNode>& Node2) const
	{
		return (Node1->F > Node2->F);
	}
};
*/

struct FPathfindingNodeCompare
{
	//Will put the lowest FScore above all
	bool operator()(const TSharedPtr<OctreeNode>& Node1, const TSharedPtr<OctreeNode>& Node2) const
	{
		return (Node1->PathfindingData->F > Node2->PathfindingData->F);
	}
};
