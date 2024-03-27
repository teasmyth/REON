// Fill out your copyright notice in the Description page of Project Settings.

#include "Pathfinding/OctreeNode.h"

LLM_DEFINE_TAG(OctreeNode);

OctreeNode::OctreeNode(const FBox& Bounds, OctreeNode* Parent, const bool ObjectsCanFloat)
{
	LLM_SCOPE_BYTAG(OctreeNode);
	NodeBounds = Bounds;
	this->Parent = Parent;
	CameFrom = nullptr;
	Occupied = false;
	NavigationNode = false;
	Floatable = ObjectsCanFloat;
	F = FLT_MAX;
	G = FLT_MAX;
	H = FLT_MAX;
}

OctreeNode::OctreeNode()
{
	LLM_SCOPE_BYTAG(OctreeNode);
	NodeBounds = FBox();
	Parent = nullptr;
	CameFrom = nullptr;
	Occupied = false;
	NavigationNode = false;
	Floatable = false;
	F = FLT_MAX;
	G = FLT_MAX;
	H = FLT_MAX;
}

OctreeNode::~OctreeNode()
{
	// Delete child nodes recursively
	
//Parent = nullptr;

	/*
	for (TSharedPtr<OctreeNode>& Child : ChildrenOctreeNodes)
	{
		if (Child != nullptr)
		{
			Child = nullptr;
		}
	}


	/*
	for (TSharedPtr<OctreeNode>& Neighbor : Neighbors)
	{
		Neighbor = nullptr;
	}

	Neighbors.Empty();
	*/
	
}

TSharedPtr<OctreeNode> OctreeNode::MakeNode(const FBox& Bounds, const TSharedPtr<OctreeNode>& Parent)
{
	return MakeShareable(new OctreeNode(Bounds, Parent.Get(), false));
}

void OctreeNode::DivideNode(const FBox& ActorBox, const float& MinSize,const float FloatAboveGroundPreference, const UWorld* World, const bool& DivideUsingBounds)
{
	TArray<TSharedPtr<OctreeNode>> NodeList;

	if (ChildrenNodeBounds.IsEmpty())
	{
		SetupChildrenBounds(FloatAboveGroundPreference);
	}

	if (!ChildrenOctreeNodes.IsEmpty())
	{
		for (const auto& Child : ChildrenOctreeNodes)
		{
			NodeList.Add(Child);
		}
	}
	
	for (int i = 0; i < NodeList.Num(); i++)
	{
		if (DivideUsingBounds)
		{
			//We only deem a node occupied if it needs no further division.
			if (NodeList[i]->Occupied && NodeList[i]->ChildrenOctreeNodes.IsEmpty()) continue;

			//Alongside checking size, if it doesn't intersect with Actor, or the complete opposite, fully contained within ActorBox, no need to divide
			const bool Intersects = NodeList[i]->NodeBounds.Intersect(ActorBox);
			const bool IsInside = ActorBox.IsInside(NodeList[i]->NodeBounds);

			if (NodeList[i]->NodeBounds.GetSize().Y <= MinSize || !Intersects || IsInside)
			{
				if (Intersects || IsInside)
				{
					NodeList[i]->Occupied = true;
				}
				continue;
			}
		}
		else
		{
			if (NodeList[i]->NodeBounds.GetSize().Y <= MinSize)
			{
				if (BoxOverlap(World, NodeList[i]->NodeBounds))
				{
					NodeList[i]->Occupied = true;
				}
				continue;
			}
		}

		if (NodeList[i]->ChildrenNodeBounds.IsEmpty())
		{
			NodeList[i]->SetupChildrenBounds(FloatAboveGroundPreference);
		}

		for (int j = 0; j < 8; j++)
		{
			if (DivideUsingBounds)
			{
				if (NodeList[i]->ChildrenNodeBounds[j].Intersect(ActorBox))
				{
					NodeList.Add(NodeList[i]->ChildrenOctreeNodes[j]);
				}
			}
			else
			{
				if (BoxOverlap(World, NodeList[i]->ChildrenNodeBounds[j]))
				{
					NodeList.Add(NodeList[i]->ChildrenOctreeNodes[j]);
				}
			}
		}
	}
}

void OctreeNode::SetupChildrenBounds(const float FloatAboveGroundPreference)
{
	ChildrenNodeBounds.SetNum(8);

	const FVector Center = NodeBounds.GetCenter();
	const float ChildSize = NodeBounds.GetSize().Y / 2.0f;

	ChildrenNodeBounds[0] = FBox(NodeBounds.Min, Center);
	ChildrenNodeBounds[1] = FBox(FVector(Center.X, NodeBounds.Min.Y, NodeBounds.Min.Z), FVector(NodeBounds.Max.X, Center.Y, Center.Z));
	ChildrenNodeBounds[2] = FBox(FVector(NodeBounds.Min.X, Center.Y, NodeBounds.Min.Z), FVector(Center.X, NodeBounds.Max.Y, Center.Z));
	ChildrenNodeBounds[3] = FBox(FVector(Center.X, Center.Y, NodeBounds.Min.Z), FVector(NodeBounds.Max.X, NodeBounds.Max.Y, Center.Z));

	ChildrenNodeBounds[4] = FBox(FVector(NodeBounds.Min.X, NodeBounds.Min.Y, Center.Z), FVector(Center.X, Center.Y, NodeBounds.Max.Z));
	ChildrenNodeBounds[5] = FBox(FVector(Center.X, NodeBounds.Min.Y, Center.Z), FVector(NodeBounds.Max.X, Center.Y, NodeBounds.Max.Z));
	ChildrenNodeBounds[6] = FBox(FVector(NodeBounds.Min.X, Center.Y, Center.Z), FVector(Center.X, NodeBounds.Max.Y, NodeBounds.Max.Z));
	ChildrenNodeBounds[7] = FBox(FVector(Center.X, Center.Y, Center.Z), NodeBounds.Max);

	ChildrenOctreeNodes.SetNum(8);

	for (int i = 0; i < 8; i++)
	{
		const bool IsFloatable = i < 4 && 1.5f * ChildSize >= FloatAboveGroundPreference || i >= 4 && ChildSize / 2.0f >= FloatAboveGroundPreference;
		ChildrenOctreeNodes[i] = MakeShareable(new OctreeNode(ChildrenNodeBounds[i], this, IsFloatable));
	}
}

bool OctreeNode::BoxOverlap(const UWorld* World, const FBox& Box)
{
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;

	TArray<FOverlapResult> OverlapResults;
	return World->OverlapMultiByObjectType(
		OverlapResults,
		Box.GetCenter(),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeBox(Box.GetExtent()),
		QueryParams
	);
}
