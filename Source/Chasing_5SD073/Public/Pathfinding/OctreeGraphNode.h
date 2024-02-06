// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"

/**
 * 
 */

LLM_DECLARE_TAG(OctreeGraphNode);

class CHASING_5SD073_API OctreeGraphNode
{
public:
	OctreeGraphNode(const FBox& Bounds);
	OctreeGraphNode();
	~OctreeGraphNode();

	FBox Bounds;
	TArray<OctreeGraphNode*> Neighbors;
	OctreeGraphNode* CameFrom;
	float F,G,H;
};
