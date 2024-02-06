// Fill out your copyright notice in the Description page of Project Settings.

#include "Pathfinding/OctreeNode.h"

LLM_DEFINE_TAG(OctreeNode);

OctreeNode::OctreeNode(const FBox& Bounds, const float& MinSize, OctreeNode* Parent)
{
	LLM_SCOPE_BYTAG(OctreeNode);
	NodeBounds = Bounds;
	this->MinSize = MinSize;
	this->Parent = Parent;
	GraphNode = new OctreeGraphNode(Bounds);

	
	ContainedActors.Empty();
	ChildrenOctreeNodes.Empty();
	ChildrenNodeBounds.Empty();
}

OctreeNode::OctreeNode()
{
	LLM_SCOPE_BYTAG(OctreeNode);
	Parent = nullptr;
	GraphNode = nullptr;
	MinSize = 0;
	ContainedActors.Empty();
	ChildrenOctreeNodes.Empty();
	ChildrenNodeBounds.Empty();
}

OctreeNode::~OctreeNode()
{
	// Delete child nodes recursively
	Parent = nullptr;
	
	for (const OctreeNode* Child : ChildrenOctreeNodes)
	{
		delete Child;
	}
	ChildrenOctreeNodes.Empty(); // Clear the array after deleting child nodes

	// Delete associated OctreeGraphNode
	if (GraphNode != nullptr)
	{
		delete GraphNode;
		GraphNode = nullptr; // Set to nullptr after deletion to avoid double-deletion
	}

	// Empty other arrays
	ChildrenNodeBounds.Empty();
	ContainedActors.Empty();
}

void OctreeNode::DivideNodeRecursively(AActor* Actor, UWorld* World)
{
	const FBox ActorBox = Actor->GetComponentsBoundingBox();

	if (NodeBounds.GetSize().Y <= MinSize)
	{
		if (AreAABBsIntersecting(NodeBounds, ActorBox))
		{
			ContainedActors.Add(Actor);
		}
		return;
	}

	// if (IsBoxInside(NodeBounds, ActorBox))
	// {
	// 	ContainedActors.Add(Actor);
	// 	return;
	// }

	SetupChildrenBounds();
	ChildrenOctreeNodes.SetNum(8);
	bool Dividing = false;

	for (int i = 0; i < 8; i++)
	{
		if (ChildrenOctreeNodes[i] == nullptr)
		{
			ChildrenOctreeNodes[i] = new OctreeNode(ChildrenNodeBounds[i], MinSize, this);
		}

		if (AreAABBsIntersecting(ChildrenNodeBounds[i], ActorBox))
		{
			Dividing = true;
			ChildrenOctreeNodes[i]->DivideNodeRecursively(Actor, World);
		}
	}

	if (!Dividing)
	{
		for (auto Element : ChildrenOctreeNodes)
		{
			delete Element;
			Element = nullptr;
		}
		ChildrenOctreeNodes.Empty();
		ChildrenNodeBounds.Empty();
	}
}

bool OctreeNode::AreAABBsIntersecting(const FBox& AABB1, const FBox& AABB2)
{
	// Check for overlap along the X-axis
	const bool OverlapX = AABB1.Min.X <= AABB2.Max.X && AABB1.Max.X >= AABB2.Min.X;

	// Check for overlap along the Y-axis
	const bool OverlapY = AABB1.Min.Y <= AABB2.Max.Y && AABB1.Max.Y >= AABB2.Min.Y;

	// Check for overlap along the Z-axis
	const bool OverlapZ = AABB1.Min.Z <= AABB2.Max.Z && AABB1.Max.Z >= AABB2.Min.Z;

	// If there's overlap along all three axes, the AABBs intersect
	return OverlapX && OverlapY && OverlapZ;
}

bool OctreeNode::IsBoxInside(const FBox& SmallBox, const FBox& BigBox)
{
	return SmallBox.Min.X >= BigBox.Min.X && SmallBox.Min.Y >= BigBox.Min.Y && SmallBox.Min.Z >= BigBox.Min.Z &&
		SmallBox.Max.X <= BigBox.Max.X && SmallBox.Max.Y <= BigBox.Max.Y && SmallBox.Max.Z <= BigBox.Max.Z;
}

void OctreeNode::SetupChildrenBounds()
{
	ChildrenNodeBounds.SetNum(8);

	const FVector Center = NodeBounds.GetCenter();

	ChildrenNodeBounds[0] = FBox(NodeBounds.Min, Center);
	ChildrenNodeBounds[1] = FBox(FVector(Center.X, NodeBounds.Min.Y, NodeBounds.Min.Z), FVector(NodeBounds.Max.X, Center.Y, Center.Z));
	ChildrenNodeBounds[2] = FBox(FVector(NodeBounds.Min.X, Center.Y, NodeBounds.Min.Z), FVector(Center.X, NodeBounds.Max.Y, Center.Z));
	ChildrenNodeBounds[3] = FBox(FVector(Center.X, Center.Y, NodeBounds.Min.Z), FVector(NodeBounds.Max.X, NodeBounds.Max.Y, Center.Z));

	ChildrenNodeBounds[4] = FBox(FVector(NodeBounds.Min.X, NodeBounds.Min.Y, Center.Z), FVector(Center.X, Center.Y, NodeBounds.Max.Z));
	ChildrenNodeBounds[5] = FBox(FVector(Center.X, NodeBounds.Min.Y, Center.Z), FVector(NodeBounds.Max.X, Center.Y, NodeBounds.Max.Z));
	ChildrenNodeBounds[6] = FBox(FVector(NodeBounds.Min.X, Center.Y, Center.Z), FVector(Center.X, NodeBounds.Max.Y, NodeBounds.Max.Z));
	ChildrenNodeBounds[7] = FBox(FVector(Center.X, Center.Y, Center.Z), NodeBounds.Max);
}

bool OctreeNode::IsVectorInsideBox(const FVector& Point, const FBox& Box)
{
	return Box.IsInside(Point);
}
