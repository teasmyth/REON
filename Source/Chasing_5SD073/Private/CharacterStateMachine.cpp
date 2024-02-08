// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterStateMachine.h"
#include "StateComponentBase.h"
#include "Kismet/GameplayStatics.h"


// Sets default values for this component's properties
UCharacterStateMachine::UCharacterStateMachine()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

// Called when the game starts
void UCharacterStateMachine::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UCharacterStateMachine::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// ...
}


bool UCharacterStateMachine::SetState(const ECharacterState& NewStateEnum)
{
	UStateComponentBase* TranslatedState = TranslateEnumToState(NewStateEnum);

	//If OnSetStateCondition returns false, it means the conditions are not meant for the new state, thus aborting switching state.
	if (TranslatedState == nullptr || !TranslatedState->OnSetStateConditionCheck(*this))
	{
		if (DebugStateMachine && GEngine)
		{
			if (TranslatedState == nullptr)
			{
				GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Red, EnumToString(NewStateEnum) + " is not assigned. Cant switch state");
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Red, "Conditions are not met for " + EnumToString(NewStateEnum));
			}
		}
		return false;
	}

	if (CurrentState != nullptr)
	{
		if (!TranslatedState->GetTransitionList()[CurrentEnumState]) return false;
		//If the current state does not allow the change to the new state, return.

		RunUpdate = false;
		//Green light! Setting new state is go!
		CurrentState->OnExitState(*this);
	}
	if (DebugStateMachine && GEngine && NewStateEnum != ECharacterState::DefaultState)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Red, "New State is: " + EnumToString(NewStateEnum));
	}

	CurrentState = TranslatedState;
	CurrentEnumState = NewStateEnum;
	CurrentState->OnEnterState(*this);
	RunUpdate = true;
	return true;
}

void UCharacterStateMachine::UpdateStateMachine()
{
	if (CurrentState != nullptr && RunUpdate)
	{
		CurrentState->OnUpdateState(*this);
	}
	OverrideDebug();
}

void UCharacterStateMachine::SetupStateMachine()
{
	if (MechanicsHierarchy.IsEmpty())
	{
		if (DebugStateMachine && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, "State machine is not properly setup. No mechanics found.");
		}
		return;
	}
	CheckForDuplicates(); //This will stop the game if there is a duplicate.
	GetComponentReferences(MechanicsHierarchy);
}

void UCharacterStateMachine::OverrideDebug() const
{
	if (MechanicsList.IsEmpty()) return;
	
	for (const auto& Mechanic : MechanicsList )
	{
		if (Mechanic.Component->GetDebugMechanic()) Mechanic.Component->OverrideDebug();
	}
}

#pragma region Override Functions For Player Code

void UCharacterStateMachine::OverrideMovementInput(FVector2d& NewMovementVector)
{
	if (CurrentState != nullptr)
	{
		CurrentState->OverrideMovementInput(*this, NewMovementVector);
	}
}

void UCharacterStateMachine::OverrideNoMovementInputEvent()
{
	if (CurrentState != nullptr)
	{
		CurrentState->OverrideNoMovementInputEvent(*this);
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



void UCharacterStateMachine::DetectStates()
{
	if (CurrentState == nullptr && DebugStateMachine)
	{
		DebugText("No current mechanical state. Automatic detection is off");
		return;
	}
	
	for (const auto Mechanic : MechanicsList)
	{
		if (CurrentState == Mechanic.Component || !CurrentState->GetTransitionList()[CurrentEnumState])
		{
			continue;
		}
		Mechanic.Component->OverrideDetectState(*this);
	}
}

#pragma endregion

#pragma region Player Helper Methods
void UCharacterStateMachine::ManualExitState()
{
	if (CurrentState != nullptr)
	{
		SetState(ECharacterState::DefaultState);
	}
}

#pragma endregion

#pragma region State Machine Helper Methods

FString UCharacterStateMachine::EnumToString(const ECharacterState& ToConvert)
{
	FString EnumAsString = UEnum::GetValueAsString(ToConvert);

	// Split the string using "::" as a delimiter and take the second part
	TArray<FString> Parts;
	EnumAsString.ParseIntoArray(Parts, TEXT("::"), true);

	if (Parts.Num() > 1)
	{
		return Parts[1];
	}

	// Return the original string if splitting fails
	return EnumAsString;
}

void UCharacterStateMachine::GetComponentReferences(const TArray<ECharacterState>& HierarchyArray)
{
	for (const ECharacterState& State : HierarchyArray)
	{
		FComponentReference NewRef;
		NewRef.PathToComponent = EnumToString(State);
		if (UStateComponentBase* Component = Cast<UStateComponentBase>(NewRef.GetComponent(GetOwner())); Component != nullptr)
		{
			MechanicsList.Add(FMechanicStateData(State, Component));
		}
		else
		{
			// Handle the case where the component reference cannot be obtained
			UE_LOG(LogTemp, Warning, TEXT("Failed to get component reference for state %s. Game is prone to crash."), *EnumToString(State));
		}
	}
}

void UCharacterStateMachine::CheckForDuplicates()
{
	TSet<ECharacterState> UniqueStates;

	for (const ECharacterState& State : MechanicsHierarchy)
	{
		if (UniqueStates.Contains(State))
		{
			// Duplicate found, display an error message
			const FString ErrorMessage = FString::Printf(TEXT("Duplicate ERROR: %s. Edit State Machine"), *EnumToString(State));
			GEngine->AddOnScreenDebugMessage(-1, 20, FColor::Red, ErrorMessage);

			// Pause the game
			UGameplayStatics::SetGamePaused(GetWorld(), true);
			return;
		}
		UniqueStates.Add(State);
	}
}

UStateComponentBase* UCharacterStateMachine::TranslateEnumToState(const ECharacterState& Enum) const
{
	// Iterate through the MechanicsHierarchy and find the matching state
	for (const auto HierarchyItem : MechanicsList)
	{
		if (HierarchyItem.State == Enum)
		{
			// Return the UStateComponentBase associated with the matching state
			return HierarchyItem.Component;
		}
	}

	// If no matching state is found, return nullptr or handle accordingly
	return nullptr;
}

void UCharacterStateMachine::DebugText(const FString& Text)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1,0, FColor::Red, Text);
	}
}
#pragma endregion
