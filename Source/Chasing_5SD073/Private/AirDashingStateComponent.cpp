// Fill out your copyright notice in the Description page of Project Settings.


#include "AirDashingStateComponent.h"

#include "GameFramework/CharacterMovementComponent.h"

// Sets default values for this component's properties
UAirDashingStateComponent::UAirDashingStateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UAirDashingStateComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}


// Called every frame
void UAirDashingStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UAirDashingStateComponent::OnEnterState(UCharacterStateMachine& SM)
{
	Super::OnEnterState(SM);
	InternalTimer = 0;
	//InitialForwardVector = PlayerCharacter->GetActorForwardVector();
	InitialForwardVector = PlayerCharacter->GetFirstPersonCameraComponent()->GetForwardVector();
}

void UAirDashingStateComponent::OnUpdateState(UCharacterStateMachine& SM)
{
	Super::OnUpdateState(SM);
	
	//Instead of relying on physics, (cause it is janky) I am manually calculating where the player is supposed to be.
	InternalTimer += GetWorld()->GetDeltaSeconds();
	const FVector NextFrameLocation = PlayerCharacter->GetActorLocation() + InitialForwardVector * AirDashDistance * (InternalTimer / AirDashTime);
	PlayerCharacter->SetActorLocation(NextFrameLocation, true); //true prevent player 'dashing' inside of a wall, stop at hitting.

	if (InternalTimer >= AirDashTime) SM.ManualExitState();
}

void UAirDashingStateComponent::OnExitState(UCharacterStateMachine& SM)
{
	Super::OnExitState(SM);
}


void UAirDashingStateComponent::OverrideMovementInput(UCharacterStateMachine& SM, FVector2d& NewMovementVector)
{
	Super::OverrideMovementInput(SM, NewMovementVector);
	//This prevents any player inputs doing air dash.
	NewMovementVector = FVector2d::ZeroVector;
}

