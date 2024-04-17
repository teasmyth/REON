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

TSharedPtr<OctreeNode> OctreeNode::LazyDivideAndFindNode(const TArray<FBox>& ActorBoxes, const float& MinSize, const float FloatAboveGroundPreference,
                                                         const FVector& Location, const bool LookingForNeighbor)
{
	if (!IsInsideNode(Location))
	{
		return nullptr;
	}

	TArray<TSharedPtr<OctreeNode>> NodeList;

	TSharedPtr<OctreeNode> ToReturn;
	TSharedPtr<OctreeNode> InsideNode;

	bool FindingClosest = false;

	if (ChildrenOctreeNodes.IsEmpty())
	{
		SetupChildrenBounds(FloatAboveGroundPreference);
		Occupied = true;
	}

	if (!ChildrenOctreeNodes.IsEmpty())
	{
		for (const auto& Child : ChildrenOctreeNodes)
		{
			if (Child->IsInsideNode(Location))
			{
				ToReturn = Child;
				break; //It cannot be in multiple children at once.
			}
		}
	}

	//This code assumes that the children of root node are divisible and not completely inside an object.
	while (ToReturn.IsValid())
	{
		if (ToReturn->ChildrenOctreeNodes.IsEmpty())
		{
			ToReturn->ChildrenOctreeNodes.SetNum(8);
		}

		for (int j = 0; j < 8; j++)
		{
			if (!ToReturn->ChildrenOctreeNodes[j].IsValid())
			{
				ToReturn->ChildrenOctreeNodes[j] = ToReturn->MakeChild(j, FloatAboveGroundPreference);
				const FVector Center = ToReturn->ChildrenOctreeNodes[j]->Position;
				const FVector Offset = FVector(ToReturn->ChildrenOctreeNodes[j]->HalfSize);
				const FBox NodeBox = FBox(Center - Offset, Center + Offset);


				for (const auto& Box : ActorBoxes)
				{
					if (NodeBox.Intersect(Box))
					{
						ToReturn->Occupied = true;
						ToReturn->ChildrenOctreeNodes[j]->IsDivisible = ToReturn->ChildrenOctreeNodes[j]->HalfSize * 2 > MinSize + 1;
						//+1 to avoid float error
						ToReturn->ChildrenOctreeNodes[j]->Occupied = true;
						break;
					}
				}


				if (ToReturn->ChildrenOctreeNodes[j]->Occupied && ToReturn->ChildrenOctreeNodes[j]->IsDivisible)
				{
					for (const auto& Box : ActorBoxes)
					{
						if (Box.IsInside(NodeBox))
						{
							ToReturn->ChildrenOctreeNodes[j]->IsDivisible = false;
							break;
						}
					}
				}
			}

			if (!FindingClosest && ToReturn->ChildrenOctreeNodes[j]->IsInsideNode(Location))
			{
				InsideNode = ToReturn->ChildrenOctreeNodes[j];
				//Not breaking on purpose because I need the other children to check if they are closer to the location.
				//In case this is occupied.
			}
			else if (InsideNode.IsValid())
			{
				if (FVector::DistSquared(ToReturn->ChildrenOctreeNodes[j]->Position, Location) < FVector::DistSquared(InsideNode->Position, Location))
				{
					InsideNode = ToReturn->ChildrenOctreeNodes[j];
				}
			}
			else InsideNode = ToReturn->ChildrenOctreeNodes[0];
		}

		ToReturn = InsideNode;

		if (!ToReturn->Occupied)
		{
			return ToReturn;
		}


		if (ToReturn->IsDivisible)
		{
			continue;
		}

		if (LookingForNeighbor) //We cannot return an occupied space for a neighbor, nor can we return a node that is the closest.
		{
			return nullptr;
		}

		TSharedPtr<OctreeNode> ClosestUnoccupied; //Plan A: if we have a close unoccupied sibling node, pick that instead
		for (const auto& Child : ToReturn->ChildrenOctreeNodes)
		{
			if (Child->Occupied) continue;

			if (!ClosestUnoccupied.IsValid()) ClosestUnoccupied = Child;

			if (FVector::DistSquared(Child->Position, Location) <= FVector::DistSquared(ClosestUnoccupied->Position, Location))
			{
				ClosestUnoccupied = Child;
			}
		}


		//In case happen to be in a very unlucky position where everything is occupied, then just use the original inside node.
		//TODO what happens when every child is occupied? Closest will be invalid, but we need to return something. Otherwise 'out of bounds'
		/*
		if (ClosestUnoccupied.IsValid())
		{
			ToReturn = ClosestUnoccupied;
		}
		
		else if (ClosestDivisible.IsValid())
		{
			ToReturn = ClosestDivisible;
			continue;
		}
		*/

		ToReturn = ClosestUnoccupied;

		//remove division, just closest, if all occupied, make the toreturn unoccpied but make sure to set it back aferwards

		//Here comes the assumption I made earlier. Apart from the children of the root node, all other children are checked if they are occupied or not
		//Thus, one of the children of NodeList[i] should be unoccupied.
		//Naturally, if the root node that is found is not divisible, we will have a weird node
		return ToReturn;
	}
	return nullptr;
}

TArray<FVector> OctreeNode::CalculatePositions(const int Face, const float& MinNodeSize) const
{
	TArray<FVector> PotentialNeighborPositions;

	// Calculate the start position of the face
	const FVector StartPosition = Position + FVector(DIRECTIONS[Face]) * HalfSize * 1.01f;

	// Calculate the number of steps in each direction
	const int Steps = FMath::RoundToInt(HalfSize * 2.0f / MinNodeSize);
	//Because if it is 1.9999 due to float error, it would be rounded to 1 by default.

	FVector Axis1, Axis2;
	if (DIRECTIONS[Face] == FIntVector(1, 0, 0) || DIRECTIONS[Face] == FIntVector(-1, 0, 0))
	{
		Axis1 = FVector(0, 1, 0);
		Axis2 = FVector(0, 0, 1);
	}
	else if (DIRECTIONS[Face] == FIntVector(0, 1, 0) || DIRECTIONS[Face] == FIntVector(0, -1, 0))
	{
		Axis1 = FVector(1, 0, 0);
		Axis2 = FVector(0, 0, 1);
	}
	else //if (DIRECTIONS[Face].Equals(FVector(0, 0, 1)) || DIRECTIONS[Face].Equals(FVector(0, 0, -1)))
	{
		Axis1 = FVector(1, 0, 0);
		Axis2 = FVector(0, 1, 0);
	}

	if (Steps == 1)
	{
		PotentialNeighborPositions.Add(StartPosition);
		return PotentialNeighborPositions;
	}

	// Iterate over the face in steps of the size of the minimum node
	for (int i = -Steps / 2; i <= Steps / 2; i++) //Because of the octree structure, steps will always be a multiple of 2. Int is safe.
	{
		if (i == 0) continue;

		for (int j = -Steps / 2; j <= Steps / 2; j++)
		{
			if (j == 0) continue;

			// Calculate the position
			const FVector PotentialNeighborPosition = StartPosition + Axis1 * i * MinNodeSize + Axis2 * j * MinNodeSize;

			// Add the position to the list
			PotentialNeighborPositions.Add(PotentialNeighborPosition);
		}
	}

	return PotentialNeighborPositions;
}


bool OctreeNode::GetNeighbors(const TSharedPtr<OctreeNode>& RootNode, const TSharedPtr<OctreeNode>& CurrentNode, const TArray<FBox>& ActorBoxes,
                              const float& MinSize,
                              const float FloatAboveGroundPreference)
{
	//Cleaning up the neighbors list from invalid pointers.
	if (CurrentNode->Neighbors.Contains(nullptr))
	{
		TSet<TWeakPtr<OctreeNode>> ValidNeighbors;
		for (const auto& Neighbor : CurrentNode->Neighbors)
		{
			if (Neighbor.IsValid())
			{
				ValidNeighbors.Add(Neighbor);
			}
		}
		CurrentNode->Neighbors = ValidNeighbors;
	}

	int NeighborCount = 0;
	TArray<TArray<FVector>> PotentialNeighborPositions;
	for (int i = 0; i < 6; i++)
	{
		PotentialNeighborPositions.Add(CurrentNode->CalculatePositions(i, MinSize));
		NeighborCount += PotentialNeighborPositions[i].Num();
	}

	//If we already have all the neighbors, no need to continue. Else, we still made useful calculation for the neighbors.
	if (CurrentNode->Neighbors.Num() == NeighborCount)
	{
		return false;
	}


	for (int i = 0; i < 6; i++)
	{
		for (const auto& Pos : PotentialNeighborPositions[i])
		{
			const TWeakPtr<OctreeNode> Node = RootNode->LazyDivideAndFindNode(ActorBoxes, MinSize, FloatAboveGroundPreference, Pos, true).ToWeakPtr();
			if (Node.IsValid() && !CurrentNode->Neighbors.Contains(Node))
			{
				CurrentNode->Neighbors.Add(Node);
				Node.Pin()->Neighbors.Add(CurrentNode.ToWeakPtr());
			}
		}
	}

	return CurrentNode->Neighbors.IsEmpty();
}

TSharedPtr<OctreeNode> OctreeNode::MakeChild(const int& ChildIndex, const float& FloatAboveGroundPreference) const
{
	const float ChildHalfSize = HalfSize / 2.0f;
	const FVector SizeVec = FVector(ChildHalfSize);

	const bool IsFloatableBotttom = 1.5f * ChildHalfSize >= FloatAboveGroundPreference;
	const bool IsFloatableTop = ChildHalfSize / 2.0f >= FloatAboveGroundPreference;

	switch (ChildIndex)
	{
	case 0: return MakeShareable(new OctreeNode(Position - SizeVec, ChildHalfSize, IsFloatableBotttom));
	case 1: return MakeShareable(new OctreeNode(FVector(Position.X + SizeVec.X, Position.Y - SizeVec.Y, Position.Z - SizeVec.Z),
	                                            ChildHalfSize, IsFloatableBotttom));
	case 2: return MakeShareable(new OctreeNode(FVector(Position.X + SizeVec.X, Position.Y + SizeVec.Y, Position.Z - SizeVec.Z),
	                                            ChildHalfSize, IsFloatableBotttom));
	case 3: return MakeShareable(new OctreeNode(FVector(Position.X - SizeVec.X, Position.Y + SizeVec.Y, Position.Z - SizeVec.Z),
	                                            ChildHalfSize, IsFloatableBotttom));

	case 4: return MakeShareable(new OctreeNode(FVector(Position.X - SizeVec.X, Position.Y - SizeVec.Y, Position.Z + SizeVec.Z),
	                                            ChildHalfSize, IsFloatableTop));
	case 5: return MakeShareable(new OctreeNode(FVector(Position.X + SizeVec.X, Position.Y - SizeVec.Y, Position.Z + SizeVec.Z),
	                                            ChildHalfSize, IsFloatableTop));
	case 6: return MakeShareable(new OctreeNode(Position + SizeVec, ChildHalfSize, IsFloatableTop));
	case 7: return MakeShareable(new OctreeNode(FVector(Position.X - SizeVec.X, Position.Y + SizeVec.Y, Position.Z + SizeVec.Z),
	                                            ChildHalfSize, IsFloatableTop));
	default: return nullptr;
	}
}

#pragma region OldStuff
bool OctreeNode::DoesNodeIntersectWithBox(const TSharedPtr<OctreeNode>& Node, const FBox& Box)
{
	const FBox NodeBox = FBox(Node->Position - FVector(Node->HalfSize), Node->Position + FVector(Node->HalfSize));
	return NodeBox.Intersect(Box);
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

TArray<FVector> OctreeNode::GetNeighborPositions(FLargeMemoryReader& OctreeData, const int64& DataIndex)
{
	TArray<FVector> ToReturn;

	OctreeData.Seek(DataIndex);

	float x, y, z, HalfSize;
	uint8 ChildCount;
	//int64 Index;

	//OctreeData << Index;
	OctreeData << x;
	OctreeData << y;
	OctreeData << z;
	OctreeData << HalfSize;
	OctreeData.SerializeBits(&ChildCount, 4);

	if (ChildCount > 0)
	{
		OctreeData << ToReturn;
	}

	return ToReturn;
}

TSharedPtr<OctreeNode> OctreeNode::LoadSingleNode(FLargeMemoryReader& OctreeData, const int64 DataIndex) //, TArray<int64>& OutIndexData)
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

	OctreeData.SerializeBits(&ChildCount, 4);

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

	//OctreeData << ChildCount;

	if (ChildCount > 0)
	{
		Node->ChildrenIndexes.SetNum(ChildCount);
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

	Ar.SerializeBits(&ChildCount, 4);

	//Ar << ChildCount;

	if (ChildCount > 0)
	{
		if (!WithSeekValues)
		{
			IndexData.Children.SetNum(ChildCount);
			IndexData.LinkedChildren.SetNum(ChildCount);
		}

		Node->ChildrenIndexes = IndexData.Children;

		if (Node->ChildrenIndexes.Num() < 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("ChildrenIndexes is less than 0"));
		}

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

	OctreeData.SerializeBits(&ChildCount, 4);

	//OctreeData << ChildCount;

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

void OctreeNode::DivideNode(const FBox& ActorBox, const float& MinSize, const float FloatAboveGroundPreference, const UWorld* World,
                            const bool& DivideUsingFBox)
{
	/*
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

	*/

	if (DivideUsingFBox)
	{
		if (Occupied && ChildrenOctreeNodes.IsEmpty()) return;

		const FBox NodeBox = FBox(Position - FVector(HalfSize), Position + FVector(HalfSize));
		const bool Intersects = NodeBox.Intersect(ActorBox);
		const bool IsInside = ActorBox.IsInside(NodeBox);

		if (HalfSize * 2 <= MinSize || !Intersects || IsInside)
		{
			if (Intersects || IsInside)
			{
				Occupied = true;
			}
			return;
		}
	}
	else if (HalfSize * 2 <= MinSize)
	{
		if (BoxOverlap(World, Position, HalfSize))
		{
			Occupied = true;
		}
		return;
	}

	if (ChildrenOctreeNodes.IsEmpty())
	{
		SetupChildrenBounds(FloatAboveGroundPreference);
		Occupied = true;
	}

	const FVector Offset = FVector(HalfSize / 2.0f);
	for (int j = 0; j < 8; j++)
	{
		const FVector Center = ChildrenOctreeNodes[j]->Position;
		if (DivideUsingFBox)
		{
			if (FBox(Center - Offset, Center + Offset).Intersect(ActorBox))
			{
				ChildrenOctreeNodes[j]->DivideNode(ActorBox, MinSize, FloatAboveGroundPreference, World, DivideUsingFBox);
			}
		}
		else
		{
			if (BoxOverlap(World, Center, ChildrenOctreeNodes[j]->HalfSize))
			{
				ChildrenOctreeNodes[j]->DivideNode(ActorBox, MinSize, FloatAboveGroundPreference, World, DivideUsingFBox);
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


#pragma endregion
