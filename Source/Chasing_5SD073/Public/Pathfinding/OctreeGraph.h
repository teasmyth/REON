// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OctreeNode.h"

class CHASING_5SD073_API OctreeGraph
{
public:
	OctreeGraph();
	~OctreeGraph();
	
	static bool LazyOctreeAStar(const bool& ThreadIsPaused, const bool& Debug, const TArray<FBox>& ActorBoxes, const float& MinSize, const FVector& StartLocation, const FVector& EndLocation, const TSharedPtr<OctreeNode>& RootNode, TArray<FVector>& OutPathList);
	
	static FVector DirectionTowardsSharedFaceFromSmallerNode(const TSharedPtr<OctreeNode>& Node1, const TSharedPtr<OctreeNode>& Node2);
	static float ManhattanDistance(const TSharedPtr<OctreeNode>& From, const TSharedPtr<OctreeNode>& To);
	static void ReconstructPath(const TSharedPtr<OctreeNode>& Start, const TSharedPtr<OctreeNode>& End, TArray<FVector>& OutPathList);

	//Checks if we have all the possible neighbors, if not, it will create them or find them. Returns true if successful, false otherwise.
	static bool GetNeighbors(const bool& ThreadIsPaused, const TSharedPtr<OctreeNode>& RootNode, const TSharedPtr<OctreeNode>& CurrentNode, const TArray<FBox>& ActorBoxes,  const float& MinSize);
	
	static TArray<double> TimeTaken;

	static TArray<FVector> CalculatePositions(const TSharedPtr<OctreeNode>& CurrentNode, const int& Face, const float& MinNodeSize);

	static void CleanupUnusedNodes(TSharedPtr<OctreeNode>& Node, const TSet<TSharedPtr<OctreeNode>>& OpenSet);

	inline static constexpr int MemoryOptimizerTickThreshold = 1000;
	inline static constexpr int MemoryCleanupFrequency = 5000;
	inline static int PathfindingMemoryTick = 0;

private: 	
	inline static FIntVector DIRECTIONS[6] = {
		FIntVector(-1, 0, 0),  // Left face
		FIntVector(1, 0, 0),   // Right face
		FIntVector(0, -1, 0),  // Front face
		FIntVector(0, 1, 0),   // Back face
		FIntVector(0, 0, -1),  // Bottom face
		FIntVector(0, 0, 1)    // Top face
	};

	static TWeakPtr<OctreeNode> PreviousValidStart;
	static TWeakPtr<OctreeNode> PreviousValidEnd;
	
};

struct FPathfindingNodeCompare
{
	//Will put the lowest FScore above all
	bool operator()(const TSharedPtr<OctreeNode>& Node1, const TSharedPtr<OctreeNode>& Node2) const
	{
		return (Node1->PathfindingData->F > Node2->PathfindingData->F);
		//I read somewhere that prioritizing also with lower H scores can help with the performance of the A* algorithm.
		//as in Node1->PathfindingData->F > Node2->PathfindingData->F && Node1->PathfindingData->H > Node2->PathfindingData->H
		//untested though
	}
};
