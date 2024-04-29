// Fill out your copyright notice in the Description page of Project Settings.

#include "Pathfinding/OctreeNode.h"

LLM_DEFINE_TAG(OctreeNode);

OctreeNode::OctreeNode(const FVector& Pos, const float HalfSize)
{
	LLM_SCOPE_BYTAG(OctreeNode);
	Position = Pos;
	this->HalfSize = HalfSize;
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

TSharedPtr<OctreeNode> OctreeNode::LazyDivideAndFindNode(const bool& ThreadIsPaused, const TArray<FBox>& ActorBoxes, const float& MinSize,
                                                         const FVector& Location, const bool LookingForNeighbor)
{
	if (!IsInsideNode(Location))
	{
		return nullptr;
	}


	TSharedPtr<OctreeNode> ToReturn;
	TSharedPtr<OctreeNode> InsideNode;

	if (ChildrenOctreeNodes.IsEmpty())
	{
		ChildrenOctreeNodes.SetNum(8);
		for (int i = 0; i < 8; i++)
		{
			ChildrenOctreeNodes[i] = MakeChild(i);
		}
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
	//If auto encapsulate is on, this should never be the case.
	//Otherwise it will return nullptr.
	while (ToReturn.IsValid() && !ThreadIsPaused)
	{
		if (ToReturn->ChildrenOctreeNodes.IsEmpty())
		{
			ToReturn->ChildrenOctreeNodes.SetNum(8);
		}

		for (int j = 0; j < 8; j++)
		{
			if (ThreadIsPaused) return nullptr;

			if (!ToReturn->ChildrenOctreeNodes[j].IsValid())
			{
				ToReturn->ChildrenOctreeNodes[j] = ToReturn->MakeChild(j);
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

			if (ToReturn->ChildrenOctreeNodes[j]->IsInsideNode(Location))
			{
				InsideNode = ToReturn->ChildrenOctreeNodes[j];
				//Not breaking on purpose because I need the other children to check if they are closer to the location.
				//Because I am not breaking, I can't do ToReturn = ToReturn->ChildrenOctreeNodes[j]; here.
			}
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

		TSharedPtr<OctreeNode> ClosestUnoccupied;
		for (const auto& Child : ToReturn->ChildrenOctreeNodes)
		{
			if (Child->Occupied) continue;

			if (!ClosestUnoccupied.IsValid()) ClosestUnoccupied = Child;

			if (FVector::DistSquared(Child->Position, Location) <= FVector::DistSquared(ClosestUnoccupied->Position, Location))
			{
				ClosestUnoccupied = Child;
			}
		}


		//ToReturn = ClosestUnoccupied;		

		if (ClosestUnoccupied.IsValid())
		{
			ToReturn = ClosestUnoccupied;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Could not find a valid node to move to. Using the original node."));
		}

		//In case happen to be in a very unlucky position where everything is occupied, then just use the original inside node.

		/* TODO MAJOR TODO. TEMPORARILY I AM USING PREVIOUS VALID NODES TO GET OUT OF THIS SITUATION.
		* IF we are here this is what happened:
		* - We are NOT looking for a neighbor. AKA we are looking for a start or end node.
		* - The node we found is OCCUPIED
		* - All of the siblings are OCCUPIED
		*
		* The issue? Because we are moving imperfectly in a perfect structure, meaning
		* We are using AddInput + Path smoothing in a rigid cubic environment, we naturally bleed into occupied spaces occasionally,
		* which is due to the fact there is a strict occupancy check, if anything is inside a particular node, it is occupied.
		* However, we must find a way to get out of this situation. Otherwise, we will be forever stuck in an infinite loop or a failed path.
		*
		* As a solution, I will let this node pass, as current node is not required to be unoccupied (in ASTAR FN!), only the neighbors.
		* So it will end up looking up the neighbors and finding the closest unoccupied node. Which it should have, given if the situation
		* described above happened (bled into an empty looking space but its 'actually' occupied) as they will for sure have neighbors.
		*
		* However, this should not be used for neighbors, as they must be unoccupied. So, in GetNeighbor where I clean invalid pointers,
		* I also remove occupied nodes from the list.
		*/
		

		return ToReturn;
	}
	return nullptr;
}

TSharedPtr<OctreeNode> OctreeNode::MakeChild(const int& ChildIndex) const
{
	const float ChildHalfSize = HalfSize / 2.0f;
	const FVector SizeVec = FVector(ChildHalfSize);

	switch (ChildIndex)
	{
	case 0: return MakeShareable(new OctreeNode(Position - SizeVec, ChildHalfSize));
	case 1: return MakeShareable(new OctreeNode(FVector(Position.X + SizeVec.X, Position.Y - SizeVec.Y, Position.Z - SizeVec.Z),
	                                            ChildHalfSize));
	case 2: return MakeShareable(new OctreeNode(FVector(Position.X + SizeVec.X, Position.Y + SizeVec.Y, Position.Z - SizeVec.Z),
	                                            ChildHalfSize));
	case 3: return MakeShareable(new OctreeNode(FVector(Position.X - SizeVec.X, Position.Y + SizeVec.Y, Position.Z - SizeVec.Z),
	                                            ChildHalfSize));

	case 4: return MakeShareable(new OctreeNode(FVector(Position.X - SizeVec.X, Position.Y - SizeVec.Y, Position.Z + SizeVec.Z),
	                                            ChildHalfSize));
	case 5: return MakeShareable(new OctreeNode(FVector(Position.X + SizeVec.X, Position.Y - SizeVec.Y, Position.Z + SizeVec.Z),
	                                            ChildHalfSize));
	case 6: return MakeShareable(new OctreeNode(Position + SizeVec, ChildHalfSize));
	case 7: return MakeShareable(new OctreeNode(FVector(Position.X - SizeVec.X, Position.Y + SizeVec.Y, Position.Z + SizeVec.Z),
	                                            ChildHalfSize));
	default: return nullptr;
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

void OctreeNode::DeleteOctreeNode(TSharedPtr<OctreeNode>& Node)
{
	// Traverse the octree from the root node to the leaf nodes
	for (TSharedPtr<OctreeNode>& Child : Node->ChildrenOctreeNodes)
	{
		if (Child != nullptr)
		{
			// Recursively delete child nodes
			DeleteOctreeNode(Child);
		}
	}

	//Is this redundant? Do I need to check for validity?
	if (Node->PathfindingData.IsValid())
	{
		Node->PathfindingData.Reset();
	}

	Node.Reset();
}
