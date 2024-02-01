// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */

struct FOctreeObject
{
	FBox Bounds;
	AActor* Actor;

	explicit FOctreeObject(AActor* InActor)
		: Bounds(InActor ? InActor->GetComponentsBoundingBox() : FBox())
		, Actor(InActor){}
};

class CHASING_5SD073_API OctreeNode
{
public:
	OctreeNode(const FBox& Bounds, const float& MinSize, OctreeNode* Parent);
	OctreeNode();
	~OctreeNode();

	int ID;
	TArray<FOctreeObject*> ContainedObjects;
	OctreeNode* Parent;
	TArray<OctreeNode*> ChildrenOctreeNodes;
	float MinSize;
	FBox NodeBounds;
	TArray<FBox> ChildrenNodeBounds;

	void DivideNode(AActor* Actor);
	void Draw();
	
};
