// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "Serialization/Archive.h"
#include "Serialization/LargeMemoryReader.h"
#include "Serialization/LargeMemoryWriter.h"

struct FCompactNodeData;
struct FPathfindingNode;



struct FIndexData
{
	//int64 Index;
	TArray<int64> Children;
	TArray<FIndexData*> LinkedChildren;

	FIndexData() = default;

	~FIndexData()
	{
		for (const auto& Child : LinkedChildren)
		{
			if (Child != nullptr) delete Child;
		}
		LinkedChildren.Empty();
	}
	

	explicit FIndexData(const uint8& ChildCount)
	{
		for (int i = 0; i < ChildCount; i++)
		{
			Children.Add(0);
		}
	}	
};

LLM_DECLARE_TAG(OctreeNode);

class CHASING_5SD073_API OctreeNode
{
public:
	OctreeNode(const FVector& Pos, const float HalfSize, const bool ObjectsCanFloat);
	OctreeNode();
	~OctreeNode();

	FVector Position;
	float HalfSize;

	int64 Index = 0;

	bool IsDivisible = true;

	TArray<int64> ChildrenIndexes;
	TArray<FVector> NeighborPositions;
	bool Occupied = false;
	bool Floatable = false; //Meaning that it is big enough that the agent will look like it is floating in it, compared to a smaller node.
	
	TSet<TWeakPtr<OctreeNode>> Neighbors;
	TArray<TSharedPtr<OctreeNode>> ChildrenOctreeNodes;
	TSharedPtr<FPathfindingNode> PathfindingData = nullptr;
	
	void DivideNode(const FBox& ActorBox, const float& MinSize, const float FloatAboveGroundPreference, const UWorld* World, const bool& DivideUsingFBox = false);
	void SetupChildrenBounds(const float FloatAboveGroundPreference);
	
	bool IsInsideNode(const FVector& Location) const;
	static bool BoxOverlap(const UWorld* World, const FVector& Center, const float& Extent);
	static bool DoNodesIntersect(const TSharedPtr<OctreeNode>& Node1, const TSharedPtr<OctreeNode>& Node2);
	static bool DoesNodeIntersectWithBox(const TSharedPtr<OctreeNode>& Node, const FBox& Box);

	static TArray<int64> GetFIndexData (FLargeMemoryReader& OctreeData, const int64& DataIndex);
	static TArray<FVector> GetNeighborPositions(FLargeMemoryReader& OctreeData,  const int64& DataIndex);

	TSharedPtr<OctreeNode> LazyDivideAndFindNode(const TArray<FBox>& ActorBoxes, const float& MinSize, const float FloatAboveGroundPreference, const FVector& Location, const bool LookingForNeighbor);
	TSharedPtr<OctreeNode> MakeChild(const int& ChildIndex, const float& FloatAboveGroundPreference) const;

	//Returns true if the Current Node's neighbors list is empty, false if it is not.
	static bool GetNeighbors(const TSharedPtr<OctreeNode>& RootNode, const TSharedPtr<OctreeNode>& CurrentNode, const TArray<FBox>& ActorBoxes, const float& MinSize, const float FloatAboveGroundPreference);
	
	
	static TSharedPtr<OctreeNode> LoadSingleNode(FLargeMemoryReader& OctreeData, const int64 DataIndex);
	static void SaveNode(FLargeMemoryWriter& Ar, FIndexData& IndexData, const TSharedPtr<OctreeNode>& Node, const bool& WithSeekValues);
	static void LoadAllNodes(FLargeMemoryReader& OctreeData, TSharedPtr<OctreeNode>& Node);

	TArray<FVector> CalculatePositions(const int Face, const float& MinNodeSize) const;
	const FIntVector DIRECTIONS[6] = {
		FIntVector(-1, 0, 0),  // Left face
		FIntVector(1, 0, 0),   // Right face
		FIntVector(0, -1, 0),  // Front face
		FIntVector(0, 1, 0),   // Back face
		FIntVector(0, 0, -1),  // Bottom face
		FIntVector(0, 0, 1)    // Top face
	};
};


struct CHASING_5SD073_API FPathfindingNode
{
	float F;
	float G;
	float H;
	TWeakPtr<OctreeNode> CameFrom;
	//TSet<TWeakPtr<OctreeNode>> Neighbors;

	//FPathfindingNode();

	FPathfindingNode()//const TSharedPtr<OctreeNode>& OctreeNode)
	{
		F = FLT_MAX;
		G = FLT_MAX;
		H = FLT_MAX;
		CameFrom = nullptr;
		//Neighbors = OctreeNode->Neighbors;
	}
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

	//3 floats are more space efficient than an FVector.
	float x, y, z;
	uint8 ChildCount;


	Ar << Node->HalfSize;

	if (Ar.IsSaving())
	{
		x = Node->Position.X;
		y = Node->Position.Y;
		z = Node->Position.Z;

		Ar << x;
		Ar << y;
		Ar << z;
		
		ChildCount = Node->ChildrenOctreeNodes.Num();

		/*Using single variable to store 3 variables is a very bad practice in terms of code readability, but it is more space efficient.
		 *In my defense, it is a very simple idea, and it is not hard to understand. I am not doing this in a complex code, so it is not a problem.
		 *Still, I am leaving this big paragraph here to explain why I did it, and to show that I am aware of the bad practice.
		 *
		 *---- Explanation
		 *
		 *First, because of Octree structure, we know for a fact that a Node can have 1-8 children.
		 *Second, if it has any children, it must be Occupied.
		 *Third, Floatable is irrelevant to any Node that is Occupied.
		 *
		 *Therefore, if a ChildCount is 1-8, Occupied will be true and I dont need to save data. Moving on..
		 *
		 *There are nodes with 0 children but still occupied, this usually happens when it had to divide but that particular Node intersects something.
		 *So, I assign the number 9 for the case
		 *
		 *With the same logic, I assign 10 for the case when It is not Occupied but is Floatable
		 *Lastly, I assign 11 for the case when it is not Occupied and not Floatable. By default both are false, however I need an 'else' statement
		 *To make sure it doesn't go to the default case (aka not leaving the child count at 0).
		 *
		 *Lastly, Because I only need numbers from 1-11, I can store them in 4 bits.
		 *This won't reduce the size of the variable, however, will make reading/saving faster.
		 *Of course, that only counts when there are lots of nodes, which there will be.
		 *Individually, the speed difference is negligible but it adds up with lots of nodes.
		 */

		if (ChildCount == 0)
		{
			if (Node->Occupied)
			{
				ChildCount = 9; //Look into this, how can it have 0 child and occupied? Should get pruned in GetEmptyNodes()
			}
			else if (Node->Floatable)
			{
				ChildCount = 10;
			}
			else ChildCount = 11;

			Ar.SerializeBits(&ChildCount, 4);
			return Ar;
		}

		Ar.SerializeBits(&ChildCount, 4);
		
	
		//Ar << Node->ChildrenOctreeNodes; Not saving TArray, saving each element individually, thus, I don't save the details of the array.
		for (auto& Child : Node->ChildrenOctreeNodes)
		{
			Ar << Child;
		}
	}
	else if (Ar.IsLoading())
	{
		Ar << x;
		Ar << y;
		Ar << z;
		Node->Position = FVector(x, y, z);

		Ar.SerializeBits(&ChildCount, 4);
		

		if (ChildCount < 9) //There is no "0" child as I overrode it in the saving. So if it is less than 9, it has a child/ren.
		{
			Node->ChildrenOctreeNodes.SetNum(ChildCount);

			for (auto& Child : Node->ChildrenOctreeNodes)
			{
				Ar << Child;
			}
			Node->Occupied = true;
		}
		else if (ChildCount == 9)
		{
			Node->Occupied = true;
		}
		else if (ChildCount == 10)
		{
			Node->Floatable = true;
		}
		//else (which would be 11) is left blank as by default, both of those booleans are false, which would be this case.
		
	}
	return Ar;
}

