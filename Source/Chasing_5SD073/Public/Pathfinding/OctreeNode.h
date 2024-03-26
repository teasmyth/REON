// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "Serialization/Archive.h"


class CHASING_5SD073_API OctreeNode
{
public:
	OctreeNode(const FBox& Bounds, OctreeNode* Parent, const bool ObjectsCanFloat);
	OctreeNode();
	~OctreeNode();

	float F;
	float G;
	float H;
	FBox NodeBounds;

	OctreeNode* Parent;
	bool Occupied = false;
	bool NavigationNode = false;
	bool Floatable = false; //Meaning that it is big enough that the agent will look like it is floating in it, compared to a smaller node.
	
	TWeakPtr<OctreeNode> CameFrom;
	TArray<TWeakPtr<OctreeNode>> Neighbors;
	TArray<TSharedPtr<OctreeNode>> ChildrenOctreeNodes;

	TArray<FVector> ChildCenters;
	TArray<FVector> NeighborCenters;
	TArray<FBox> ChildrenNodeBounds;

	static TSharedPtr<OctreeNode> MakeNode(const FBox& Bounds, const TSharedPtr<OctreeNode>& Parent);


	void DivideNode(const FBox& ActorBox, const float& MinSize, const float FloatAboveGroundPreference, const UWorld* World,
	                const bool& DivideUsingBounds = false);
	void SetupChildrenBounds(const float FloatAboveGroundPreference);

	static bool BoxOverlap(const UWorld* World, const FBox& Box);
};

inline FArchive& operator <<(FArchive& Ar, TSharedPtr<OctreeNode>& Node)
{
	if (Node == nullptr)
	{
		if (Ar.IsLoading())
		{
			Node = MakeShareable(new OctreeNode());
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
	Ar << Node->Floatable;

	int Size = Node->ChildrenOctreeNodes.Num();
	Ar << Size;
	if (Ar.IsLoading())
	{
		Node->ChildrenOctreeNodes.SetNum(Size);
	}
	Ar << Node->ChildrenOctreeNodes;


	return Ar;
}
