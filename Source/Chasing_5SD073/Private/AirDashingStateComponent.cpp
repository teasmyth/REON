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

bool UAirDashingStateComponent::OnSetStateConditionCheck(UCharacterStateMachine& SM)
{
	return PlayerCharacter->GetHorizontalVelocity() > 0 && !PlayerCharacter->GetCharacterStateMachine()->IsThisCurrentState(*this);
}

void UAirDashingStateComponent::OnEnterState(UCharacterStateMachine& SM)
{
	Super::OnEnterState(SM);
	InternalTimer = 0;
	InitialForwardVector = PlayerCharacter->GetFirstPersonCameraComponent()->GetComponentRotation().Vector();
	const float Speed = PlayerCharacter->GetHorizontalVelocity();
	PlayerMovement->Velocity = FVector(InitialForwardVector.X * Speed, InitialForwardVector.Y * Speed, PlayerMovement->Velocity.Z);
}

void UAirDashingStateComponent::OnUpdateState(UCharacterStateMachine& SM)
{
	Super::OnUpdateState(SM);

	//Instead of relying on physics, (cause it is janky) I am manually calculating where the player is supposed to be.
	InternalTimer += GetWorld()->GetDeltaSeconds();
	const FVector NextFrameLocation = PlayerCharacter->GetActorLocation() + InitialForwardVector * AirDashDistance * (InternalTimer / AirDashTime) *
		(PlayerCharacter->GetHorizontalVelocity() / PlayerCharacter->GetMaxRunningSpeed()); //Last part provides boost on current speed
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
	//This prevents any player inputs doing air dash.v
	NewMovementVector = FVector2d::ZeroVector;
}

void UAirDashingStateComponent::OverrideDebug()
{
	if (!DebugMechanic) return;
	
	Super::OverrideDebug();
	const FVector Start = GetOwner()->GetActorLocation();
	DrawDebugLine(GetWorld(), Start, Start + GetOwner()->GetActorForwardVector() * AirDashDistance * (PlayerCharacter->GetHorizontalVelocity() /
		              PlayerCharacter->GetMaxRunningSpeed()), DebugColor, false, 0, 0, 3);
}
