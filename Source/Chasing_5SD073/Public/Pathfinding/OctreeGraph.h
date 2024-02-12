// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OctreeNode.h"

/**
 * 
 */

class CHASING_5SD073_API OctreeGraph
{
public:
	OctreeGraph();
	~OctreeGraph();

	TArray<OctreeNode*> Nodes;
	TArray<OctreeNode*> RootNodes;

	void AddNode(OctreeNode* Node);
	void AddRootNode(OctreeNode* Node);

	void ConnectNodes();
	bool OctreeAStar(const FVector& StartLocation, const FVector& EndLocation, TArray<FVector>& OutPathList);
	static void ReconstructPath(const OctreeNode* Start, const OctreeNode* End, TArray<FVector>& OutPathList);
	static FVector DirectionTowardsSharedFaceFromSmallerNode(const OctreeNode* Node1, const OctreeNode* Node2);
	static float ManhattanDistance(const OctreeNode* From, const OctreeNode* To);
	OctreeNode* FindGraphNode(const FVector& Location);

private:
	TArray<FVector> Directions
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
	bool operator()(const OctreeNode* Node1, const OctreeNode* Node2) const
	{
		return (Node1->F > Node2->F);
	}
};
