// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OctreeGraphNode.h"

LLM_DECLARE_TAG(OctreeNode);

class CHASING_5SD073_API OctreeNode
{
public:
	OctreeNode(const FBox& Bounds, const float& MinSize, OctreeNode* Parent);
	OctreeNode();
	~OctreeNode();

	TArray<AActor*> ContainedActors;
	OctreeNode* Parent;
	TArray<OctreeNode*> ChildrenOctreeNodes;
	OctreeGraphNode* GraphNode;
	

	float MinSize;
	FBox NodeBounds;
	TArray<FBox> ChildrenNodeBounds;

	void DivideNodeRecursively(AActor* Actor, UWorld* World);
	static bool AreAABBsIntersecting(const FBox& AABB1, const FBox& AABB2);
	static bool IsBoxInside(const FBox& SmallBox, const FBox& BigBox);
	void SetupChildrenBounds();
	static bool IsVectorInsideBox(const FVector& Point, const FBox& Box);
};
