// Fill out your copyright notice in the Description page of Project Settings.

#include "Pathfinding/OctreeNode.h"

OctreeNode::OctreeNode(const FBox& Bounds, const float& MinSize, OctreeNode* Parent, int& ID)
{
	NodeBounds = Bounds;
	this->MinSize = MinSize;
	this->ID = ID;
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
	const FVector Center = NodeBounds.GetCenter();
	/*
	ChildrenNodeBounds[0] = FBox(NodeBounds.Min, Center);
	ChildrenNodeBounds[1] = FBox(FVector(Center.X, NodeBounds.Min.Y, NodeBounds.Min.Z), FVector(NodeBounds.Max.X, Center.Y, Center.Z));
	ChildrenNodeBounds[2] = FBox(FVector(NodeBounds.Min.X, Center.Y, NodeBounds.Min.Z), FVector(Center.X, NodeBounds.Max.Y, Center.Z));
	ChildrenNodeBounds[3] = FBox(FVector(Center.X, Center.Y, NodeBounds.Min.Z), FVector(NodeBounds.Max.X, NodeBounds.Max.Y, Center.Z));

	ChildrenNodeBounds[4] = FBox(FVector(NodeBounds.Min.X, NodeBounds.Min.Y, Center.Z), FVector(Center.X, Center.Y, NodeBounds.Max.Z));
	ChildrenNodeBounds[5] = FBox(FVector(Center.X, NodeBounds.Min.Y, Center.Z), FVector(NodeBounds.Max.X, Center.Y, NodeBounds.Max.Z));
	ChildrenNodeBounds[6] = FBox(FVector(NodeBounds.Min.X, Center.Y, Center.Z), FVector(Center.X, NodeBounds.Max.Y, NodeBounds.Max.Z));
	ChildrenNodeBounds[7] = FBox(Center, NodeBounds.Max);*/



	ChildrenNodeBounds[0] = FBox(NodeBounds.Min, Center);
	ChildrenNodeBounds[1] = FBox(FVector(Center.X, NodeBounds.Min.Y, NodeBounds.Min.Z), FVector(NodeBounds.Max.X, Center.Y, Center.Z));
	ChildrenNodeBounds[2] = FBox(FVector(NodeBounds.Min.X, Center.Y, NodeBounds.Min.Z), FVector(Center.X, NodeBounds.Max.Y, Center.Z));
	ChildrenNodeBounds[3] = FBox(FVector(Center.X, Center.Y, NodeBounds.Min.Z), FVector(NodeBounds.Max.X, NodeBounds.Max.Y, Center.Z));

	ChildrenNodeBounds[4] = FBox(FVector(NodeBounds.Min.X, NodeBounds.Min.Y, Center.Z), FVector(Center.X, Center.Y, NodeBounds.Max.Z));
	ChildrenNodeBounds[5] = FBox(FVector(Center.X, NodeBounds.Min.Y, Center.Z), FVector(NodeBounds.Max.X, Center.Y, NodeBounds.Max.Z));
	ChildrenNodeBounds[6] = FBox(FVector(NodeBounds.Min.X, Center.Y, Center.Z),FVector(Center.X, NodeBounds.Max.Y, NodeBounds.Max.Z));
	ChildrenNodeBounds[7] = FBox(FVector(Center.X, Center.Y, Center.Z),NodeBounds.Max);

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
		DrawDebugBox(World, Actor->GetComponentsBoundingBox().GetCenter(), Actor->GetComponentsBoundingBox().GetExtent(), FQuat::Identity,
						 FColor::Red, false, 15, 0, 3);
		ContainedActors.Add(Actor);
		return;
	}

	bool Dividing = false;

	for (int i = 0; i < 8; i++)
	{
		if (ChildrenOctreeNodes[i] == nullptr)
		{
			ChildrenOctreeNodes[i] = new OctreeNode(ChildrenNodeBounds[i], MinSize, this, ID);
		}
		//if (ChildrenNodeBounds[i].Intersect(Actor->GetComponentsBoundingBox()))

		const FBox ActorBox = Actor->GetComponentsBoundingBox();
		const FBox ChildrenBox = ChildrenNodeBounds[i];
		
		if (AreAABBsIntersecting(ChildrenBox, ActorBox) || AreAABBsIntersecting(ActorBox, ChildrenBox))
		{
			ID++;
			UE_LOG(LogTemp, Warning, TEXT("Total nodes: %i"), ID);
			Dividing = true;
			MaxRecursion--;
			if (MaxRecursion > 0) ChildrenOctreeNodes[i]->DivideNodeRecursively(Actor, World, MaxRecursion);
			DrawDebugBox(World, ChildrenOctreeNodes[i]->NodeBounds.GetCenter(), ChildrenOctreeNodes[i]->NodeBounds.GetExtent(), FColor::Green, false,
			             15, 0, 2);
		}
	}

	if (!Dividing)
	{
		ContainedActors.Add(Actor);
		DrawDebugBox(World, Actor->GetComponentsBoundingBox().GetCenter(), Actor->GetComponentsBoundingBox().GetExtent(), FQuat::Identity,
						 FColor::Red, false, 15, 0, 3);
		DrawDebugBox(World, NodeBounds.GetCenter(), NodeBounds.GetExtent(), FColor::Blue, false, 15, 0, 2);
		//ChildrenOctreeNodes.Empty();
		//UE_LOG(LogTemp, Warning, TEXT("Node not divided. Objects: %d"), ContainedObjects.Num());
	}
}

void OctreeNode::Draw()
{
}

void OctreeNode::GenerateChildren()
{
}

bool OctreeNode::AreAABBsIntersecting(const FBox& AABB1, const FBox& AABB2)
{
	// Check for overlap along the X-axis
	bool overlapX = AABB1.Min.X <= AABB2.Max.X && AABB1.Max.X >= AABB2.Min.X;

	// Check for overlap along the Y-axis
	bool overlapY = AABB1.Min.Y <= AABB2.Max.Y && AABB1.Max.Y >= AABB2.Min.Y;

	// Check for overlap along the Z-axis
	bool overlapZ = AABB1.Min.Z <= AABB2.Max.Z && AABB1.Max.Z >= AABB2.Min.Z;

	// If there's overlap along all three axes, the AABBs intersect
	return overlapX && overlapY && overlapZ;
}
