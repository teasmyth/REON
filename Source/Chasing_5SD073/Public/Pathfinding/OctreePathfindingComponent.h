// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <mutex>

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OctreePathfindingComponent.generated.h"


class AOctree;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CHASING_5SD073_API UOctreePathfindingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UOctreePathfindingComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


private:
	FVector PreviousNextLocation = FVector::ZeroVector;

	std::unique_ptr<std::thread> PathfindingThread;
	std::mutex PathfindingMutex;
	bool IsPathfindingInProgress = false;
};
