// Fill out your copyright notice in the Description page of Project Settings.


#include "StateComponentBase.h"

// Sets default values for this component's properties
UStateComponentBase::UStateComponentBase()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	//Adding all the possible states by default to Possible Transitions
	UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("ECharacterState"), true);
	if (EnumPtr)
	{
		// Get the maximum valid enum value
		const ECharacterState MaxEnumValue = static_cast<ECharacterState>(EnumPtr->GetMaxEnumValue());

		for (int32 EnumIndex = 0; EnumIndex < EnumPtr->NumEnums(); ++EnumIndex)
		{
			ECharacterState EnumValue = static_cast<ECharacterState>(EnumPtr->GetValueByIndex(EnumIndex));

			// Skip the maximum valid enum value. By default UE adds a 'MAX' value to UEnums, which I don't want or need on the list.
			if (EnumValue == MaxEnumValue)
			{
				continue;
			}

			CanTransitionFromStateList.Add(EnumValue, true); // Set the default value to false
		}
	}
	// ...
}


// Called when the game starts
void UStateComponentBase::BeginPlay()
{
	Super::BeginPlay();

	PlayerCapsule = GetOwner()->GetComponentByClass<UCapsuleComponent>();
	PlayerMovement = GetOwner()->GetComponentByClass<UCharacterMovementComponent>();
	PlayerCharacter = Cast<AMyCharacter>(GetOwner());
	// ...
	
}


// Called every frame
void UStateComponentBase::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UStateComponentBase::OnEnterState(UCharacterStateMachine& SM)
{
	OnEnterStateDelegate.Broadcast();
}

void UStateComponentBase::OnUpdateState(UCharacterStateMachine& SM)
{
	OnUpdateStateDelegate.Broadcast();
}

void UStateComponentBase::OnExitState(UCharacterStateMachine& SM)
{
	OnExitStateDelegate.Broadcast();
}

void UStateComponentBase::OverrideMovementInput(FVector2d& NewMovementVector)
{
	
}

void UStateComponentBase::OverrideAcceleration(float& NewSpeed)
{
}

void UStateComponentBase::OverrideCameraInput(UCameraComponent& Camera, FVector2d& NewRotationVector)
{
}


