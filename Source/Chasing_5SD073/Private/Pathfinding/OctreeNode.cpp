// Fill out your copyright notice in the Description page of Project Settings.

#include "Pathfinding/OctreeNode.h"

#include "Serialization/StaticMemoryReader.h"

LLM_DEFINE_TAG(OctreeNode);

OctreeNode::OctreeNode(const FVector& Pos, const float HalfSize, const bool ObjectsCanFloat)
{
	LLM_SCOPE_BYTAG(OctreeNode);
	Position = Pos;
	this->HalfSize = HalfSize;
	Floatable = ObjectsCanFloat;
}

OctreeNode::OctreeNode()
{
	LLM_SCOPE_BYTAG(OctreeNode);
	Position = FVector::ZeroVector;
	HalfSize = 0;
}

OctreeNode::~OctreeNode()
{
	PathfindingData.Reset();
}

void OctreeNode::DivideNode(const FBox& ActorBox, const float& MinSize, const float FloatAboveGroundPreference, const UWorld* World,
                            const bool& DivideUsingFBox)
{
	TArray<TSharedPtr<OctreeNode>> NodeList;

	if (ChildrenOctreeNodes.IsEmpty())
	{
		SetupChildrenBounds(FloatAboveGroundPreference);
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
			const FBox NodeBox = FBox(NodeList[i]->Position - FVector(NodeList[i]->HalfSize), NodeList[i]->Position + FVector(NodeList[i]->HalfSize));
			const bool Intersects = NodeBox.Intersect(ActorBox);
			const bool IsInside = ActorBox.IsInside(NodeBox);

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
			if (NodeList[i]->HalfSize * 2 <= MinSize)
			{
				if (BoxOverlap(World, NodeList[i]->Position, NodeList[i]->HalfSize))
				{
					NodeList[i]->Occupied = true;
				}
				continue;
			}
		}

		if (NodeList[i]->ChildrenOctreeNodes.IsEmpty())
		{
			NodeList[i]->SetupChildrenBounds(FloatAboveGroundPreference);
			NodeList[i]->Occupied = true; //If we have children, then we cannot use this node, therefore, it is "occupied".
		}

		const FVector Offset = FVector(NodeList[i]->ChildrenOctreeNodes[0]->HalfSize);
		for (int j = 0; j < 8; j++)
		{
			if (DivideUsingFBox)
			{
				const FVector Center = NodeList[i]->ChildrenOctreeNodes[j]->Position;
				if (FBox(Center - Offset, Center + Offset).Intersect(ActorBox))
				{
					NodeList.Add(NodeList[i]->ChildrenOctreeNodes[j]);
				}
			}
			else
			{
				if (BoxOverlap(World, NodeList[i]->ChildrenOctreeNodes[j]->Position, NodeList[i]->ChildrenOctreeNodes[j]->HalfSize))
				{
					NodeList.Add(NodeList[i]->ChildrenOctreeNodes[j]);
				}
			}
		}
	}
}

void OctreeNode::SetupChildrenBounds(const float FloatAboveGroundPreference)
{
	const float ChildHalfSize = HalfSize / 2.0f;
	const FVector SizeVec = FVector(ChildHalfSize);

	const bool IsFloatableBotttom = 1.5f * ChildHalfSize >= FloatAboveGroundPreference;
	const bool IsFloatableTop = ChildHalfSize / 2.0f >= FloatAboveGroundPreference;

	ChildrenOctreeNodes.SetNum(8);

	ChildrenOctreeNodes[0] = MakeShareable(new OctreeNode(Position - SizeVec, ChildHalfSize, IsFloatableBotttom));
	ChildrenOctreeNodes[1] = MakeShareable(new OctreeNode(FVector(Position.X + SizeVec.X, Position.Y - SizeVec.Y, Position.Z - SizeVec.Z),
	                                                      ChildHalfSize, IsFloatableBotttom));
	ChildrenOctreeNodes[2] = MakeShareable(new OctreeNode(FVector(Position.X + SizeVec.X, Position.Y + SizeVec.Y, Position.Z - SizeVec.Z),
	                                                      ChildHalfSize, IsFloatableBotttom));
	ChildrenOctreeNodes[3] = MakeShareable(new OctreeNode(FVector(Position.X - SizeVec.X, Position.Y + SizeVec.Y, Position.Z - SizeVec.Z),
	                                                      ChildHalfSize, IsFloatableBotttom));

	ChildrenOctreeNodes[4] = MakeShareable(new OctreeNode(FVector(Position.X - SizeVec.X, Position.Y - SizeVec.Y, Position.Z + SizeVec.Z),
	                                                      ChildHalfSize, IsFloatableTop));
	ChildrenOctreeNodes[5] = MakeShareable(new OctreeNode(FVector(Position.X + SizeVec.X, Position.Y - SizeVec.Y, Position.Z + SizeVec.Z),
	                                                      ChildHalfSize, IsFloatableTop));
	ChildrenOctreeNodes[6] = MakeShareable(new OctreeNode(Position + SizeVec, ChildHalfSize, IsFloatableTop));
	ChildrenOctreeNodes[7] = MakeShareable(new OctreeNode(FVector(Position.X - SizeVec.X, Position.Y + SizeVec.Y, Position.Z + SizeVec.Z),
	                                                      ChildHalfSize, IsFloatableTop));
}

bool OctreeNode::BoxOverlap(const UWorld* World, const FVector& Center, const float& Extent)
{
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;

	TArray<FOverlapResult> OverlapResults;
	return World->OverlapMultiByObjectType(
		OverlapResults,
		Center,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeBox(FVector(Extent)),
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

TArray<int64> OctreeNode::GetFIndexData(FLargeMemoryReader& OctreeData, const int64& DataIndex)
{
	TArray<int64> ToReturn;

	OctreeData.Seek(DataIndex);

	float x, y, z, HalfSize;
	uint8 ChildCount;
	//int64 Index;

	//OctreeData << Index;
	OctreeData << x;
	OctreeData << y;
	OctreeData << z;
	OctreeData << HalfSize;
	OctreeData << ChildCount;

	if (ChildCount > 0)
	{
		OctreeData << ToReturn;
	}

	return ToReturn;
}

TSharedPtr<OctreeNode> OctreeNode::LoadSingleNode(FLargeMemoryReader& OctreeData, const int64 DataIndex)//, TArray<int64>& OutIndexData)
{
	OctreeData.Seek(DataIndex);
	const TSharedPtr<OctreeNode> Node = MakeShareable(new OctreeNode());

	float x, y, z;
	uint8 ChildCount;

	//OctreeData << Node->Index;
	OctreeData << x;
	OctreeData << y;
	OctreeData << z;
	OctreeData << Node->HalfSize;
	Node->Position = FVector(x, y, z);

	//OctreeData.SerializeBits(&ChildCount, 4);
	
	/*
	if (ChildCount < 9) //There is no "0" child as I overrode it in the saving. So if it is less than 9, it has a child/ren.
	{
		OctreeData << OutIndexData;
		Node->ChildrenOctreeNodes.SetNum(ChildCount);
		Node->Occupied = true;
		return Node;
	}

	if (ChildCount == 9)
	{
		Node->Occupied = true;
	}
	else if (ChildCount == 10)
	{
		Node->Floatable = true;
	}

	OutIndexData.Children.Empty();
	*/

	OctreeData << ChildCount;

	if (ChildCount > 0)
	{
		OctreeData << Node->ChildrenIndexes;
		Node->ChildrenOctreeNodes.SetNum(ChildCount);
		Node->Occupied = true;
	}
	else
	{
		Node->ChildrenOctreeNodes.Empty();
		Node->ChildrenIndexes.Empty();
		OctreeData << Node->NeighborPositions;
		OctreeData << Node->Floatable;
		//OctreeData << Node->Occupied;
		//Node->Occupied 
	}

	return Node;
}

void OctreeNode::SaveNode(FLargeMemoryWriter& Ar, FIndexData& IndexData, const TSharedPtr<OctreeNode>& Node, const bool& WithSeekValues)
{
	if (Node == nullptr)
	{
		return;
	}

	float x = Node->Position.X;
	float y = Node->Position.Y;
	float z = Node->Position.Z;
	uint8 ChildCount = Node->ChildrenOctreeNodes.Num();

	Node->Index = Ar.Tell();
	//Ar << Node->Index;
	Ar << x;
	Ar << y;
	Ar << z;
	Ar << Node->HalfSize;

	/*
	if (ChildCount == 0)
	{
		if (Node->Occupied)
		{
			ChildCount = 9; //todo Look into this, how can it have 0 child and occupied? Should get pruned in GetEmptyNodes()
		}
		else if (Node->Floatable)
		{
			ChildCount = 10;
		}
		else ChildCount = 11;

		Ar.SerializeBits(&ChildCount, 4);
		return;
	}

	Ar.SerializeBits(&ChildCount, 4);

	if (!WithSeekValues)
	{
		IndexData.Children.SetNum(ChildCount);
		IndexData.LinkedChildren.SetNum(ChildCount);
	}
	
	Ar << IndexData;

	for (int i = 0; i < Node->ChildrenOctreeNodes.Num(); i++)
	{
		if (!WithSeekValues)
		{
			IndexData.Children[i] = Ar.Tell();
			IndexData.LinkedChildren[i] = new FIndexData();
		}
		SaveNode(Ar, *IndexData.LinkedChildren[i], Node->ChildrenOctreeNodes[i], WithSeekValues);
	}
	*/

	//Ar.SerializeBits(&ChildCount, 4);

	Ar << ChildCount;

	if (ChildCount > 0)
	{
		
		if (!WithSeekValues)
		{
			IndexData.Children.SetNum(ChildCount);
			IndexData.LinkedChildren.SetNum(ChildCount);
		}

		Node->ChildrenIndexes = IndexData.Children;
	
		Ar << Node->ChildrenIndexes;


		for (int i = 0; i < Node->ChildrenOctreeNodes.Num(); i++)
		{
			if (!WithSeekValues)
			{
				IndexData.LinkedChildren[i] = new FIndexData();
			}
			
			SaveNode(Ar, *IndexData.LinkedChildren[i], Node->ChildrenOctreeNodes[i], WithSeekValues);

			if (!WithSeekValues)
			{
				IndexData.Children[i] = Node->ChildrenOctreeNodes[i]->Index;
			}
		}
	}
	else
	{
		Ar << Node->NeighborPositions;
		Ar << Node->Floatable;
		//Ar << Node->Occupied;
	}
}

void OctreeNode::LoadAllNodes(FLargeMemoryReader& OctreeData, TSharedPtr<OctreeNode>& Node)
{
	if (!Node.IsValid())
	{
		Node = MakeShareable(new OctreeNode());
	}
	
	float x, y, z;
	uint8 ChildCount;

	//OctreeData << Node->Index;
	OctreeData << x;
	OctreeData << y;
	OctreeData << z;
	OctreeData << Node->HalfSize;
	Node->Position = FVector(x, y, z);
	
	//OctreeData.SerializeBits(&ChildCount, 4);

	OctreeData << ChildCount;

	if (ChildCount > 0)
	{
		OctreeData << Node->ChildrenIndexes;
		Node->ChildrenOctreeNodes.SetNum(ChildCount);
		for (auto& Child : Node->ChildrenOctreeNodes)
		{
			LoadAllNodes(OctreeData, Child);
		}
		Node->Occupied = true;
	}
	else
	{
		OctreeData << Node->NeighborPositions;
		OctreeData << Node->Floatable;
		//OctreeData << Node->Occupied;
	}
}

bool OctreeNode::IsInsideNode(const FVector& Location) const
{
	const FVector MinPoint = Position - HalfSize;
	const FVector MaxPoint = Position + HalfSize;

	return (Location.X >= MinPoint.X && Location.X <= MaxPoint.X) &&
		(Location.Y >= MinPoint.Y && Location.Y <= MaxPoint.Y) &&
		(Location.Z >= MinPoint.Z && Location.Z <= MaxPoint.Z);
}
