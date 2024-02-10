// Fill out your copyright notice in the Description page of Project Settings.

#include "Pathfinding/OctreeNode.h"

LLM_DEFINE_TAG(OctreeNode);

OctreeNode::OctreeNode(const FBox& Bounds, OctreeNode* Parent)
{
	LLM_SCOPE_BYTAG(OctreeNode);
	NodeBounds = Bounds;
	this->Parent = Parent;
	CameFrom = nullptr;
	GraphNode = nullptr;
	F = FLT_MAX;
	G = FLT_MAX;
	H = FLT_MAX;

	//ContainedActors.Empty();
	ChildrenOctreeNodes.Empty();
	ChildrenNodeBounds.Empty();
}

OctreeNode::OctreeNode()
{
	LLM_SCOPE_BYTAG(OctreeNode);
	Parent = nullptr;
	GraphNode = nullptr;
	CameFrom = nullptr;
	F = FLT_MAX;
	G = FLT_MAX;
	H = FLT_MAX;
	//ContainedActors.Empty();
	ChildrenOctreeNodes.Empty();
	ChildrenNodeBounds.Empty();
}

OctreeNode::~OctreeNode()
{
	// Delete child nodes recursively
	Parent = nullptr;
	//GraphNode deletion is handled in OctreeGraph

	for (const OctreeNode* Child : ChildrenOctreeNodes)
	{
		if (Child != nullptr)
		{
			delete Child;
		}
	}
	ChildrenOctreeNodes.Empty();
	ChildrenNodeBounds.Empty();
	//ContainedActors.Empty();
}

void OctreeNode::DivideNode(const AActor* Actor, const float& MinSize)
{
	const FBox ActorBox = Actor->GetComponentsBoundingBox();

	TArray<OctreeNode*> NodeList;
	NodeList.Add(this);

	for (int i = 0; i < NodeList.Num(); i++)

	{
		if (NodeList[i]->Occupied) continue; //We only deem a node occupied if it needs no further division.
		
		const bool Intersects = NodeList[i]->NodeBounds.Intersect(ActorBox);
		const bool IsInside = ActorBox.IsInside(NodeList[i]->NodeBounds);

		//Alongside checking size, if it doesn't intersect with Actor, or the complete opposite, fully contained within ActorBox, no need to divide
		if (NodeList[i]->NodeBounds.GetSize().Y <= MinSize || !Intersects || IsInside)
		{
			if (Intersects || IsInside)
			{
				//NodeList[i]->ContainedActors.Add(Actor);
				NodeList[i]->ChildrenOctreeNodes.Empty();
				NodeList[i]->ChildrenNodeBounds.Empty();
				NodeList[i]->Occupied = true;
			}
			continue;
		}
		
		NodeList[i]->SetupChildrenBounds();
		NodeList[i]->ChildrenOctreeNodes.SetNum(8);
		bool Dividing = false;

		for (int j = 0; j < 8; j++)
		{
			if (NodeList[i]->ChildrenOctreeNodes[j] == nullptr)
			{
				NodeList[i]->ChildrenOctreeNodes[j] = new OctreeNode(NodeList[i]->ChildrenNodeBounds[j], NodeList[i]);
			}


			if (NodeList[i]->ChildrenNodeBounds[j].Intersect(ActorBox))
			{
				NodeList.Add(NodeList[i]->ChildrenOctreeNodes[j]);
				Dividing = true;
			}
		}

		//Though this would make more sense at the last added object instead of deleting and remaking the same thing at every passed Actor
		if (!Dividing)
		{
			//Because I pass this function multiple times with every actor, it might have divided in previous cases but not now.
			//In that case, I shouldn't be deleting the children.
			bool DividedPreviously = false;
			for (const auto Element : NodeList[i]->ChildrenOctreeNodes)
			{
				if (!Element->ChildrenOctreeNodes.IsEmpty())
				{
					DividedPreviously = true;
				}
			}

			if (!DividedPreviously)
			{
				for (const auto Element : NodeList[i]->ChildrenOctreeNodes)
				{
					delete Element;
				}
				NodeList[i]->ChildrenOctreeNodes.Empty();
				NodeList[i]->ChildrenNodeBounds.Empty();
			}
		}
	}
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
