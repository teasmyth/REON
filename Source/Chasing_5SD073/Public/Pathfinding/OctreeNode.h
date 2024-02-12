// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"


class CHASING_5SD073_API OctreeNode
{
public:
	OctreeNode(const FBox& Bounds, OctreeNode* Parent);
	OctreeNode();
	~OctreeNode();

	bool Occupied = false;
	float F,G,H;
	OctreeNode* CameFrom;
	TArray<OctreeNode*> Neighbors;
	//TArray<AActor*> ContainedActors; //This should be the approach if we have moving obstacles.

	
	OctreeNode* Parent; //For my current needs, it is redundant. Decided to keep it anyway.
	FBox NodeBounds;
	TArray<OctreeNode*> ChildrenOctreeNodes;
	TArray<FBox> ChildrenNodeBounds;

	void DivideNode(const AActor* Actor, const float& MinSize);
	void SetupChildrenBounds();

};
