// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OctreeGraphNode.h"
#include "OctreeNode.h"

/**
 * 
 */

class CHASING_5SD073_API OctreeGraph
{
public:
	OctreeGraph();
	~OctreeGraph();

	TArray<OctreeGraphNode*> Nodes;
	TArray<OctreeNode*> RootNodes;

	void AddNode(OctreeGraphNode* Node);
	void AddRootNode(OctreeNode* Node);

	
	bool OctreeAStar(const FVector& StartLocation, const FVector& EndLocation, TArray<FVector>& OutPathList);
	void ReconstructPath(const OctreeGraphNode* Start, const OctreeGraphNode* End, TArray<FVector>& OutPathList);
	OctreeGraphNode* FindGraphNode(const FVector& Location);
};

struct FOctreeGraphNodeCompare
{
	//Will put the lowest FScore above all
	bool operator()(const OctreeGraphNode* Node1, const OctreeGraphNode* Node2) const
	{
		return (Node1->F > Node2->F);
	}
};
