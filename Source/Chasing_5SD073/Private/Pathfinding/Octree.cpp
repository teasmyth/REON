// Fill out your copyright notice in the Description page of Project Settings.


#include "Pathfinding/Octree.h"

AOctree::AOctree()
{
	TArray<FOverlapResult> Result;
	const FVector Size = FVector(DivisionsX, DivisionsY, DivisionsZ);
	const FBox Bounds = FBox(GetActorLocation() - Size, GetActorLocation() + Size);
	GetWorld()->OverlapMultiByChannel(Result, GetActorLocation(), FQuat::Identity, ECC_GameTraceChannel1, FCollisionShape::MakeBox(Size));
	RootNode = OctreeNode(Bounds, MinNodeSize, nullptr);

	
}

void AOctree::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	//Here where I will visualize the size
}

void AOctree::AddObjects(TArray<FOverlapResult> FoundObjects)
{
	for (auto Hit : FoundObjects)
	{
		RootNode.DivideNode(Hit.GetActor());
	}
}
