// Fill out your copyright notice in the Description page of Project Settings.

#include "Pathfinding/OctreeNode.h"

OctreeNode::OctreeNode(const FBox& Bounds, const float& MinSize, OctreeNode* Parent)
{
	NodeBounds = Bounds;
	this->MinSize = MinSize;
	ID = 0;
	this->Parent = Parent;

	const float QuarterSize = NodeBounds.GetSize().Y / 4.0f;
	const float HalfSize = NodeBounds.GetSize().Y / 2.0f;

	const FVector Directions[8] = {
		FVector(-1, 1, -1), FVector(1, 1, -1),
		FVector(-1, 1, 1), FVector(1, 1, 1),
		FVector(-1, -1, -1), FVector(1, -1, -1),
		FVector(-1, -1, 1), FVector(1, -1, 1)
	};

	ChildrenNodeBounds.SetNum(8);
	ChildrenOctreeNodes.SetNum(8);

	for (int i = 0; i < 8; ++i)
	{
		ChildrenNodeBounds[i] = FBox(NodeBounds.GetCenter() - Directions[i] * QuarterSize - FVector(QuarterSize),
		                             NodeBounds.GetCenter() + Directions[i] * HalfSize);
	}
}

OctreeNode::OctreeNode()
{
}

OctreeNode::~OctreeNode()
{
	for (auto Child : ChildrenOctreeNodes)
	{
		delete Child;
	}

	//ChildrenOctreeNodes.Empty();

	for (auto Object : ContainedObjects)
	{
		delete Object;
	}

	ContainedObjects.Empty();
}

void OctreeNode::DivideNodeRecursively(AActor* Actor, UWorld* World, int& MaxRecursion)
{
	if (NodeBounds.GetSize().Y / 2.0f <= MinSize)
	{
		UE_LOG(LogTemp, Warning, TEXT("Node size: %f"), NodeBounds.GetSize().Y / 2.0f);
		ContainedActors.Add(Actor);
		return;
	}

	bool Dividing = false;

	for (int i = 0; i < 8; i++)
	{
		if (ChildrenOctreeNodes[i] == nullptr)
		{
			ChildrenOctreeNodes[i] = new OctreeNode(ChildrenNodeBounds[i], MinSize, this);
		}
		if (ChildrenNodeBounds[i].Intersect(Actor->GetComponentsBoundingBox()))
		{
			Dividing = true;
			MaxRecursion--;
			if (MaxRecursion > 0) ChildrenOctreeNodes[i]->DivideNodeRecursively(Actor, World, MaxRecursion);
			
		}
		DrawDebugBox(World, ChildrenOctreeNodes[i]->NodeBounds.GetCenter(), ChildrenOctreeNodes[i]->NodeBounds.GetExtent(), FColor::Green, false,
		             15, 0, 2);
	}

	if (!Dividing)
	{
		ContainedActors.Add(Actor);
		DrawDebugBox(World, NodeBounds.GetCenter(), NodeBounds.GetExtent(), FColor::Blue, false, 15, 0, 2);
		//ChildrenOctreeNodes.Empty();
		UE_LOG(LogTemp, Warning, TEXT("Node not divided. Objects: %d"), ContainedObjects.Num());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Node divided."));
	}
}

void OctreeNode::Draw()
{
}

void OctreeNode::GenerateChildren()
{
}
