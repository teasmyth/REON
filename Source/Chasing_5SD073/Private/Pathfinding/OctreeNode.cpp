// Fill out your copyright notice in the Description page of Project Settings.


#include "Pathfinding/OctreeNode.h"

OctreeNode::OctreeNode(const FBox& Bounds, const float& MinSize, OctreeNode* Parent)
{
	NodeBounds = Bounds;
	this->MinSize = MinSize;
	ID = 0;
	this->Parent = Parent;

	float QuarterSize = NodeBounds.GetSize().Y / 4.0f;
	FVector ChildSize = FVector(NodeBounds.GetSize().Y / 2.0f);

	ChildrenNodeBounds.SetNum(8);
	FVector Directions[8] = {
		FVector(-1, 1, -1), FVector(1, 1, -1),
		FVector(-1, 1, 1), FVector(1, 1, 1),
		FVector(-1, -1, -1), FVector(1, -1, -1),
		FVector(-1, -1, 1), FVector(1, -1, 1)
	};

	for (int i = 0; i < 8; ++i)
	{
		ChildrenNodeBounds[i] = FBox(NodeBounds.GetCenter() + Directions[i] * QuarterSize,
		                             NodeBounds.GetCenter() + Directions[i] * QuarterSize + ChildSize);
	}
}

OctreeNode::OctreeNode()
{
}

OctreeNode::~OctreeNode()
{
}

void OctreeNode::DivideNode(AActor* Actor)
{
	FOctreeObject OctObj = FOctreeObject(Actor);
	if (NodeBounds.GetSize().Y <= MinSize)
	{
		ContainedObjects.Push(&OctObj);
		return;
	}

	if (ChildrenOctreeNodes.IsEmpty())
	{
		ChildrenOctreeNodes.SetNum(8);
	}

	bool Dividing = false;

	for (int i = 0; i < 8; i++)
	{
		if (ChildrenOctreeNodes[i] == nullptr)
		{
			ChildrenOctreeNodes[i] = new OctreeNode(ChildrenNodeBounds[i], MinSize, this);
		}

		if (ChildrenNodeBounds[i].Intersect(OctObj.Bounds))
		{
			Dividing = true;
			ChildrenOctreeNodes[i]->DivideNode(Actor);
		}
	}
	
	if (!Dividing)
	{
		ContainedObjects.Add(&OctObj);
		delete &ChildrenOctreeNodes;
	}
}

void OctreeNode::Draw()
{
	
}
