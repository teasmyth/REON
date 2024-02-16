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
	int ParentID = 0;
	int ID = 0;
	TArray<int> ChildIDs;
	TArray<int> NeighborIDs;
	bool Occupied = false;
	OctreeNode* CameFrom;
	TArray<OctreeNode*> Neighbors;
	//TArray<AActor*> ContainedActors; //This should be the approach if we have moving obstacles.


	OctreeNode* Parent; //For my current needs, it is redundant. Decided to keep it anyway.

	TArray<OctreeNode*> ChildrenOctreeNodes;
	TArray<FBox> ChildrenNodeBounds;

	void DivideNode(const AActor* Actor, const float& MinSize, const UWorld* World, const bool& DivideUsingBounds = false);
	void SetupChildrenBounds();

	bool BoxOverlap(const UWorld* World, const FBox& Box);


	void SaveLoadData(FArchive& Ar, OctreeNode* Node)
	{
		Ar << Node->F;
		Ar << Node->G;
		Ar << Node->H;
		Ar << Node->NodeBounds;
		Ar << Node->ParentID;
		Ar << Node->ID;
		Ar << Node->ChildIDs;
		Ar << Node->NeighborIDs;
	}
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
	
	Ar << Node->F;
	Ar << Node->G;
	Ar << Node->H;
	Ar << Node->NodeBounds;
	Ar << Node->ParentID;
	Ar << Node->ID;


	//UE_LOG(LogTemp, Warning, TEXT("Before serialization: Node->ID = %d, Node->ChildIDs = %d"), Node->ID, Node->ChildIDs.Num());

	if (Ar.IsSaving())
	{
		Ar << Node->ChildIDs;
		Ar << Node->NeighborIDs;
	}
	else if (Ar.IsLoading())
	{
		Ar << Node->ChildIDs;
		Ar << Node->NeighborIDs;
		
	}

	//	UE_LOG(LogTemp, Warning, TEXT("After serialization/deserialization: Node->ID = %d, Node->ChildIDs.Num() = %d"), Node->ID, Node->ChildIDs.Num());

	return Ar;
}
