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

	InternalTimerStart = FApp::GetCurrentTime();
	bIsDashing = true;
	bDashed = false;

	InitialLocation = PlayerCharacter->GetActorLocation();

	PlayerMovement->MaxFlySpeed= AirDashSpeedBoost;
}

void UAirDashingStateComponent::OnUpdateState(UCharacterStateMachine& SM)
{
	Super::OnUpdateState(SM);
	if (FApp::GetCurrentTime() - InternalTimerStart >= DashTimeFrame)
	{
		//SM.ResetState();
	}


	if (FVector::Dist(InitialLocation, PlayerCharacter->GetActorLocation()) >= AirDashDistance)
	{
		PlayerMovement->SetMovementMode(MOVE_Walking);
		//PlayerMovement->Velocity = FVector(0,0,0);
		PlayerMovement->GravityScale = 1;
		SM.ResetState();
	}
}

void UAirDashingStateComponent::OnExitState(UCharacterStateMachine& SM)
{
	Super::OnExitState(SM);
	bIsDashing = false;
	bDashed = false;
}

void UAirDashingStateComponent::OverrideMovement(float& NewSpeed)
{
	//PlayerCharacter->SetCharacterSpeed(PlayerCharacter->GetMaxRunningSpeed() * AirDashSpeedBoost, true);

	PlayerMovement->GravityScale = 0;
	PlayerMovement->SetMovementMode(MOVE_Flying);
	//PlayerMovement->Launch(PlayerCharacter->GetActorRotation().Vector() * AirDashSpeedBoost);
	//PlayerMovement->SetMovementMode(MOVE_Walking);
	bDashed = true;
}

void UAirDashingStateComponent::OverrideMovementInputSensitivity(FVector2d& NewMovementVector)
{
	//PlayerCharacter->SetCharacterSpeed(PlayerCharacter->GetMaxRunningSpeed() * AirDashSpeedBoost, true);
	//bIsDashing = true;
}
