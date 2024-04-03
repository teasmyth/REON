// Fill out your copyright notice in the Description page of Project Settings.

#include "Pathfinding/OctreeNode.h"

LLM_DEFINE_TAG(OctreeNode);

OctreeNode::OctreeNode(const FBox& Bounds, const bool ObjectsCanFloat)
{
	LLM_SCOPE_BYTAG(OctreeNode);
	//NodeBounds = Bounds;
	//CameFrom = nullptr;
	Occupied = false;
	Floatable = ObjectsCanFloat;
	Position = Bounds.GetCenter();
	HalfSize = Bounds.GetSize().Y / 2.0f;
	
}

OctreeNode::OctreeNode(const FVector& Pos, const float HalfSize, const bool ObjectsCanFloat)
{
	LLM_SCOPE_BYTAG(OctreeNode);
	Position = Pos;
	this->HalfSize = HalfSize;
	Floatable = ObjectsCanFloat;
	//NodeBounds = FBox(Pos - FVector(HalfSize), Pos + FVector(HalfSize));
}

OctreeNode::OctreeNode()
{
	LLM_SCOPE_BYTAG(OctreeNode);
	//NodeBounds = FBox();

	//CameFrom = nullptr;
	Occupied = false;
	Floatable = false;
	//F = FLT_MAX;
	//G = FLT_MAX;
	//H = FLT_MAX;
}

OctreeNode::~OctreeNode()
{
	for (TSharedPtr<OctreeNode>& Child : ChildrenOctreeNodes)
	{
		if (Child != nullptr)
		{
			Child.Reset();
		}
	}

	PathfindingData.Reset();
}

TSharedPtr<OctreeNode> OctreeNode::MakeNode(const FBox& Bounds)
{
	return MakeShareable(new OctreeNode(Bounds, false));
}

void OctreeNode::DivideNode(const FBox& ActorBox, const float& MinSize, const float FloatAboveGroundPreference, const UWorld* World,
                            const bool& DivideUsingFBox)
{
	TArray<TSharedPtr<OctreeNode>> NodeList;

	if (ChildrenOctreeNodes.IsEmpty())
	{
		TestSetupChildrenBounds(FloatAboveGroundPreference);
		Occupied = true;
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
		if (DivideUsingFBox)
		{
			//We only deem a node occupied if it needs no further division.
			if (NodeList[i]->Occupied && NodeList[i]->ChildrenOctreeNodes.IsEmpty()) continue;

			//Alongside checking size, if it doesn't intersect with Actor, or the complete opposite, fully contained within ActorBox, no need to divide
			//const bool Intersects = NodeList[i]->NodeBounds.Intersect(ActorBox);
			const FBox NodeBox = FBox(NodeList[i]->Position - FVector(NodeList[i]->HalfSize) ,NodeList[i]->Position + FVector(NodeList[i]->HalfSize));
			const bool Intersects = NodeBox.Intersect(ActorBox);
			//const bool IsInside = ActorBox.IsInside(NodeList[i]->NodeBounds);
			const bool IsInside = ActorBox.IsInside(NodeBox);

			//if (NodeList[i]->NodeBounds.GetSize().Y <= MinSize || !Intersects || IsInside)
			if (NodeList[i]->HalfSize * 2 <= MinSize || !Intersects || IsInside)
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
			//if (NodeList[i]->NodeBounds.GetSize().Y <= MinSize)
			if (NodeList[i]->HalfSize <= MinSize)
			{
				//if (BoxOverlap(World, NodeList[i]->NodeBounds))

				/*
				if (BoxOverlap(World, NodeList[i]->NodeBounds))
				{
					NodeList[i]->Occupied = true;
				}
				*/
				
				continue;
			}
		}

		if (NodeList[i]->ChildrenOctreeNodes.IsEmpty())
		{
			NodeList[i]->TestSetupChildrenBounds(FloatAboveGroundPreference);
			NodeList[i]->Occupied = true; //If we have children, then we cannot use this node, therefore, it is "occupied".
		}

		const FVector Offset = FVector(NodeList[i]->ChildrenOctreeNodes[0]->HalfSize);
		for (int j = 0; j < 8; j++)
		{
			if (DivideUsingFBox)
			{
				//if (NodeList[i]->ChildrenOctreeNodes[j]->NodeBounds.Intersect(ActorBox))
				const FVector Center = NodeList[i]->ChildrenOctreeNodes[j]->Position;
				if (FBox(Center - Offset, Center + Offset).Intersect(ActorBox))
				{
					NodeList.Add(NodeList[i]->ChildrenOctreeNodes[j]);
				}
			}
			
			else
			{
				/*
				if (BoxOverlap(World, NodeList[i]->ChildrenOctreeNodes[j]->NodeBounds))
				{
					NodeList.Add(NodeList[i]->ChildrenOctreeNodes[j]);
				}
				*/
			}
			
		}
	}
}

void OctreeNode::SetupChildrenBounds(const float FloatAboveGroundPreference)
{
	/*
	TArray<FBox> ChildrenNodeBounds;
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
		ChildrenOctreeNodes[i] = MakeShareable(new OctreeNode(ChildrenNodeBounds[i], IsFloatable));
	}/
	*/
	
}

void OctreeNode::TestSetupChildrenBounds(const float FloatAboveGroundPreference)
{
	const float ChildHalfSize = HalfSize / 2.0f;
	const FVector SizeVec = FVector(ChildHalfSize);

	const bool IsFloatableBotttom =  1.5f * ChildHalfSize >= FloatAboveGroundPreference;
	const bool IsFloatableTop = ChildHalfSize / 2.0f >= FloatAboveGroundPreference;

	ChildrenOctreeNodes.SetNum(8);

	ChildrenOctreeNodes[0] = MakeShareable(new OctreeNode(Position - SizeVec, ChildHalfSize, IsFloatableBotttom));
	ChildrenOctreeNodes[1] = MakeShareable(new OctreeNode(FVector(Position.X + SizeVec.X, Position.Y - SizeVec.Y, Position.Z - SizeVec.Z), ChildHalfSize, IsFloatableBotttom));
	ChildrenOctreeNodes[2] = MakeShareable(new OctreeNode(FVector(Position.X + SizeVec.X, Position.Y + SizeVec.Y, Position.Z - SizeVec.Z), ChildHalfSize, IsFloatableBotttom));
	ChildrenOctreeNodes[3] = MakeShareable(new OctreeNode(FVector(Position.X - SizeVec.X, Position.Y + SizeVec.Y, Position.Z - SizeVec.Z), ChildHalfSize, IsFloatableBotttom));
	
	ChildrenOctreeNodes[4] = MakeShareable(new OctreeNode(FVector(Position.X - SizeVec.X, Position.Y - SizeVec.Y, Position.Z + SizeVec.Z), ChildHalfSize, IsFloatableTop));
	ChildrenOctreeNodes[5] = MakeShareable(new OctreeNode(FVector(Position.X + SizeVec.X, Position.Y - SizeVec.Y, Position.Z + SizeVec.Z), ChildHalfSize, IsFloatableTop));
	ChildrenOctreeNodes[6] = MakeShareable(new OctreeNode(Position + SizeVec, ChildHalfSize, IsFloatableTop));
	ChildrenOctreeNodes[7] = MakeShareable(new OctreeNode(FVector(Position.X - SizeVec.X, Position.Y + SizeVec.Y, Position.Z + SizeVec.Z), ChildHalfSize, IsFloatableTop));
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

bool OctreeNode::DoNodesIntersect(const TSharedPtr<OctreeNode>& Node1, const TSharedPtr<OctreeNode>& Node2)
{
	// Create bounding boxes for each node
	const FBox Box1 = FBox(Node1->Position - FVector(Node1->HalfSize), Node1->Position + FVector(Node1->HalfSize));
	const FBox Box2 = FBox(Node2->Position - FVector(Node2->HalfSize), Node2->Position + FVector(Node2->HalfSize));

	// Check if the boxes intersect
	return Box1.Intersect(Box2);
}

bool OctreeNode::IsInsideNode(const TSharedPtr<OctreeNode>& Node, const FVector& Location)
{
	
	const float HalfSize = Node->HalfSize;
	const FVector MinPoint = Node->Position - FVector(HalfSize);
	const FVector MaxPoint = Node->Position + FVector(HalfSize);

	return (Location.X >= MinPoint.X && Location.X <= MaxPoint.X) &&
		   (Location.Y >= MinPoint.Y && Location.Y <= MaxPoint.Y) &&
		   (Location.Z >= MinPoint.Z && Location.Z <= MaxPoint.Z);

	

	const FBox Box = FBox(Node->Position - FVector(Node->HalfSize), Node->Position + FVector(Node->HalfSize));
	return Box.IsInside(Location);
}

