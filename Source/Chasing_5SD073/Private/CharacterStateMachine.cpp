// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterStateMachine.h"

#include "AirDashingStateComponent.h"
#include "FallingStateComponent.h"
#include "IdleStateComponent.h"
#include "WalkingStateComponent.h"
#include "RunningStateComponent.h"
#include "SlidingStateComponent.h"
#include "WallClimbingStateComponent.h"
#include "WallRunningStateComponent.h"


// Sets default values for this component's properties
UCharacterStateMachine::UCharacterStateMachine()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	
	// ...
}

void UCharacterStateMachine::SetupStates()
{
	Idle = GetOwner()->GetComponentByClass<UIdleStateComponent>();
	Walking = GetOwner()->GetComponentByClass<UWalkingStateComponent>();
	Running = GetOwner()->GetComponentByClass<URunningStateComponent>();
	Sliding = GetOwner()->GetComponentByClass<USlidingStateComponent>();
	WallClimbing = GetOwner()->GetComponentByClass<UWallClimbingStateComponent>();
	WallRunning = GetOwner()->GetComponentByClass<UWallRunningStateComponent>();
	AirDashing = GetOwner()->GetComponentByClass<UAirDashingStateComponent>();
	Falling = GetOwner()->GetComponentByClass<UFallingStateComponent>();
}


// Called when the game starts
void UCharacterStateMachine::BeginPlay()
{
	Super::BeginPlay();
	SetupStates();
	SetState(ECharacterState::Sliding);

	// ...
}



// Called every frame
void UCharacterStateMachine::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CurrentState != nullptr && RunUpdate)
	{
		CurrentState->OnUpdateState();
	}
	// ...
}

UStateComponentBase* UCharacterStateMachine::TranslateEnumToState(const ECharacterState& Enum) const
{
	switch (Enum)
	{
	case ECharacterState::Idle: return Idle;
	case ECharacterState::Walking: return Walking;
	case ECharacterState::Running: return Running;
	case ECharacterState::Sliding: return Sliding;
	case ECharacterState::WallClimbing: return WallClimbing;
	case ECharacterState::WallRunning: return WallRunning;
	case ECharacterState::AirDashing: return AirDashing;
	case ECharacterState::Falling: return Falling;
	default: return nullptr;
	}
}


void UCharacterStateMachine::SetState(const ECharacterState& NewStateEnum)
{
	//Making sure the translation is valid
	if (TranslateEnumToState(NewStateEnum) == nullptr) return;
	
	
	if (CurrentState != nullptr)
	{
		if (!CurrentState->CanTransitionFromStateList[NewStateEnum]) return; //If the current state does not allow the change to the new state, return.

		//Green light! Setting new state is go!
		
		RunUpdate = false;
		CurrentState->OnExitState();
	}

	
	CurrentState = TranslateEnumToState(NewStateEnum);
	CurrentEnumState = NewStateEnum;
	CurrentState->OnEnterState();
	RunUpdate = true;
}


