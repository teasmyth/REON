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

	TArray<AActor*> ContainedActors; //could be just bool tbh
	OctreeNode* Parent;
	TArray<OctreeNode*> ChildrenOctreeNodes;
	OctreeGraphNode* GraphNode;

	FBox NodeBounds;
	TArray<FBox> ChildrenNodeBounds;

	void DivideNode(AActor* Actor, const float& MinSize);
	void SetupChildrenBounds();

};
