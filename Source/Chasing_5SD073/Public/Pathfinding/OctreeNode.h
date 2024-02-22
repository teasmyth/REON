// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "Serialization/Archive.h"


class CHASING_5SD073_API OctreeNode
{
public:
	OctreeNode(const FBox& Bounds, OctreeNode* Parent);
	OctreeNode();
	~OctreeNode();

	float F;
	float G;
	float H;
	FBox NodeBounds;
	
	OctreeNode* Parent;
	bool Occupied = false;
	bool NavigationNode = false;
	OctreeNode* CameFrom;
	TArray<OctreeNode*> Neighbors;
	TArray<OctreeNode*> ChildrenOctreeNodes;

	TArray<FVector> ChildCenters;
	TArray<FVector> NeighborCenters;
	TArray<FBox> ChildrenNodeBounds;


	void DivideNode(const FBox& ActorBox, const float& MinSize, const UWorld* World, const bool& DivideUsingBounds = false);
	void SetupChildrenBounds();

	static bool BoxOverlap(const UWorld* World, const FBox& Box);
};

inline FArchive& operator <<(FArchive& Ar, OctreeNode*& Node)
{
	if (Node == nullptr)
	{
		if (Ar.IsLoading())
		{
			Node = new OctreeNode();
		}
		if (Ar.IsSaving())
		{
			return Ar;
		}
	}

	Ar << Node->NodeBounds;
	Ar << Node->ChildCenters;
	Ar << Node->NeighborCenters;
	Ar << Node->NavigationNode;


	int Size = Node->ChildrenOctreeNodes.Num();
	Ar << Size;
	if (Ar.IsLoading())
	{
		Node->ChildrenOctreeNodes.SetNum(Size);
	}
	Ar << Node->ChildrenOctreeNodes;


	return Ar;
}
