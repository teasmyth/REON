// Fill out your copyright notice in the Description page of Project Settings.


#include "Pathfinding/Octree.h"

AOctree::AOctree()
{
}

void AOctree::BeginPlay()
{
	Super::BeginPlay();
	
	TArray<FOverlapResult> Result;
	const FVector Size = FVector(DivisionsX);
	const FBox Bounds = FBox(GetActorLocation() - Size / 2, GetActorLocation() + Size / 2);
	GetWorld()->OverlapMultiByChannel(Result, GetActorLocation(), FQuat::Identity, ECC_GameTraceChannel1, FCollisionShape::MakeBox(Size / 2));

	for (auto OverlapResult : Result)
	{
		//DrawDebugSphere(GetWorld(), OverlapResult.GetActor()->GetActorLocation(), 20, 4, FColor::Red, false, 15, 0, 2);
		//Bounds += OverlapResult.GetActor()->GetComponentsBoundingBox();
	}
	RootNode = new OctreeNode(Bounds, MinNodeSize, nullptr);
	UE_LOG(LogTemp, Warning, TEXT("Found objects: %i"), Result.Num());
	AddObjects(Result);
}

void AOctree::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	//Here where I will visualize the size
}

void AOctree::AddObjects(TArray<FOverlapResult> FoundObjects)
{
	UWorld* World = GetWorld();
	int MaxRecursion = 30;
	for (auto Hit : FoundObjects)
	{
		RootNode->DivideNodeRecursively(Hit.GetActor(), World, MaxRecursion);
	}
}
