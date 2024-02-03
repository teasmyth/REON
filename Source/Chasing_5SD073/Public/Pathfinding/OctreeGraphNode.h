// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class CHASING_5SD073_API OctreeGraphNode
{
public:
	OctreeGraphNode();
	~OctreeGraphNode();

	TArray<OctreeGraphNode*> Neighbors;
	OctreeGraphNode* CameFrom;
	float F,G,H;
};
