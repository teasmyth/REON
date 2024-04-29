// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "Serialization/Archive.h"
#include "Serialization/LargeMemoryReader.h"
#include "Serialization/LargeMemoryWriter.h"

struct FPathfindingNode;

LLM_DECLARE_TAG(OctreeNode);

class CHASING_5SD073_API OctreeNode
{
public:
	OctreeNode(const FVector& Pos, const float HalfSize);
	OctreeNode();
	~OctreeNode();

	FVector Position;
	float HalfSize;
	
	bool IsDivisible = true;
	bool Occupied = false;

	int MemoryOptimizerTick = 0;
	bool NodeIsInUse = false;
	
	//TSet<TWeakPtr<OctreeNode>> Neighbors;
	TArray<TSharedPtr<OctreeNode>> ChildrenOctreeNodes;
	TSharedPtr<FPathfindingNode> PathfindingData = nullptr;
	
	bool IsInsideNode(const FVector& Location) const;
	TSharedPtr<OctreeNode> LazyDivideAndFindNode(const bool& ThreadIsPaused, const TArray<FBox>& ActorBoxes, const float& MinSize, const FVector& Location, const bool LookingForNeighbor);
	TSharedPtr<OctreeNode> MakeChild(const int& ChildIndex) const;
	static void DeleteOctreeNode(TSharedPtr<OctreeNode>& Node);
	static void DeleteUnusedNodes(TSharedPtr<OctreeNode>& Node, const int& MemoryOptimizerTickThreshold);
};


struct CHASING_5SD073_API FPathfindingNode
{
	float F;
	float G;
	float H;
	TWeakPtr<OctreeNode> CameFrom;
	TSet<TWeakPtr<OctreeNode>> Neighbors;
	
	FPathfindingNode()
	{
		F = FLT_MAX;
		G = FLT_MAX;
		H = FLT_MAX;
		CameFrom = nullptr;
	}
};

