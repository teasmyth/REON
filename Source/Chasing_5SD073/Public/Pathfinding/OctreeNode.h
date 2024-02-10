// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <future>

#include "CoreMinimal.h"
#include "OctreeGraphNode.h"

LLM_DECLARE_TAG(OctreeNode);

class CHASING_5SD073_API OctreeNode
{
public:
	OctreeNode(const FBox& Bounds, OctreeNode* Parent);
	OctreeNode();
	~OctreeNode();

	bool Occupied = false;
	//TArray<AActor*> ContainedActors; //This should be the approach if we have moving obstacles.
	OctreeNode* Parent;
	TArray<OctreeNode*> ChildrenOctreeNodes;
	OctreeGraphNode* GraphNode;
	OctreeNode* CameFrom;

	FBox NodeBounds;
	TArray<FBox> ChildrenNodeBounds;

	float F,G,H;

	void DivideNode(const AActor* Actor, const float& MinSize);
	void SetupChildrenBounds();

};
