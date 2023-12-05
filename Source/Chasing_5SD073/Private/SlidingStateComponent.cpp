// Fill out your copyright notice in the Description page of Project Settings.


#include "SlidingStateComponent.h"

// Sets default values for this component's properties
USlidingStateComponent::USlidingStateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void USlidingStateComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}


// Called every frame
void USlidingStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void USlidingStateComponent::OnEnterState()
{
	Super::OnEnterState();
	GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, "State Machine Entered");
	
}

void USlidingStateComponent::OnUpdateState()
{
	Super::OnUpdateState();
	
}

void USlidingStateComponent::OnExitState()
{
	Super::OnExitState();
}




