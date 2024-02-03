// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OctreeGraphNode.h"

/**
 * 
 */

class OctreeNode;

class CHASING_5SD073_API OctreeGraph
{
public:
	OctreeGraph();
	~OctreeGraph();

	TArray<OctreeGraphNode*> Nodes;

	void AddNode(OctreeGraphNode* Node);
	void AddEdge(OctreeNode* From, OctreeNode* To);
	void OctreeAStar(OctreeNode* StartNode, OctreeNode* EndNode, TArray<OctreeNode*>& OutPathList);
	void ReconstructPath(OctreeGraphNode Start, OctreeGraphNode End, TArray<OctreeGraphNode*> OutPathList);
};
