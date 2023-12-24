// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterStateMachine.h"

#include "AirDashingStateComponent.h"
#include "NoneStateComponent.h"
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
	DefaultState = GetOwner()->GetComponentByClass<UNoneStateComponent>();
	Sliding = GetOwner()->GetComponentByClass<USlidingStateComponent>();
	WallClimbing = GetOwner()->GetComponentByClass<UWallClimbingStateComponent>();
	WallRunning = GetOwner()->GetComponentByClass<UWallRunningStateComponent>();
	AirDashing = GetOwner()->GetComponentByClass<UAirDashingStateComponent>();
}


// Called when the game starts
void UCharacterStateMachine::BeginPlay()
{
	Super::BeginPlay();
	SetupStates();

	// ...
}


// Called every frame
void UCharacterStateMachine::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//Debug();
	// ...
}

UStateComponentBase* UCharacterStateMachine::TranslateEnumToState(const ECharacterState& Enum) const
{
	switch (Enum)
	{
	case ECharacterState::DefaultState: return DefaultState;
	case ECharacterState::Sliding: return Sliding;
	case ECharacterState::WallClimbing: return WallClimbing;
	case ECharacterState::WallRunning: return WallRunning;
	case ECharacterState::AirDashing: return AirDashing;
	default: return nullptr;
	}
}


void UCharacterStateMachine::SetState(const ECharacterState& NewStateEnum)
{
	//Making sure the translation is valid
	UStateComponentBase* TranslatedState = TranslateEnumToState(NewStateEnum);

	//If OnSetStateCondition returns false, it means the conditions are not meant for the new state, thus aborting switching state.
	if (TranslatedState == nullptr || TranslatedState != nullptr && !TranslatedState->OnSetStateConditionCheck(*this)) return;
	
	if (CurrentState != nullptr)
	{
		if (!TranslatedState->GetTransitionList()[CurrentEnumState]) return;
		//If the current state does not allow the change to the new state, return.

		//Green light! Setting new state is go!
		RunUpdate = false;
		CurrentState->OnExitState(*this);
	}
	
	CurrentState = TranslatedState;
	CurrentEnumState = NewStateEnum;
	CurrentState->OnEnterState(*this);
	RunUpdate = true;
}

void UCharacterStateMachine::UpdateStateMachine()
{
	if (CurrentState != nullptr && RunUpdate)
	{
		CurrentState->OnUpdateState(*this);
	}
}

void UCharacterStateMachine::OverrideMovementInput(FVector2d& NewMovementVector)
{
	if (CurrentState != nullptr)
	{
		CurrentState->OverrideMovementInput(*this, NewMovementVector);
	}
}

void UCharacterStateMachine::OverrideAcceleration(float& NewSpeed)
{
	if (CurrentState != nullptr)
	{
		CurrentState->OverrideAcceleration(*this, NewSpeed);
	}
}

void UCharacterStateMachine::OverrideCameraInput(FVector2d& NewRotationVector)
{
	if (CurrentState != nullptr)
	{
		CurrentState->OverrideCameraInput(*this, NewRotationVector);
	}
}

void UCharacterStateMachine::ManualExitState()
{
	if (CurrentState != nullptr)
	{
		SetState(ECharacterState::DefaultState);
	}
}


