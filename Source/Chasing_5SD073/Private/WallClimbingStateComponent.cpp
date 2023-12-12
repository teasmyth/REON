// Fill out your copyright notice in the Description page of Project Settings.


#include "WallClimbingStateComponent.h"

// Sets default values for this component's properties
UWallClimbingStateComponent::UWallClimbingStateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UWallClimbingStateComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UWallClimbingStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UWallClimbingStateComponent::OnEnterState(UCharacterStateMachine& SM)
{
	Super::OnEnterState(SM);
}

void UWallClimbingStateComponent::OnUpdateState(UCharacterStateMachine& SM)
{
	Super::OnUpdateState(SM);
}

void UWallClimbingStateComponent::OnExitState(UCharacterStateMachine& SM)
{
	Super::OnExitState(SM);
}

