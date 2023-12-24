// Fill out your copyright notice in the Description page of Project Settings.


#include "SlidingStateComponent.h"

#include "GameFramework/CharacterMovementComponent.h"

// Sets default values for this component's properties
USlidingStateComponent::USlidingStateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void USlidingStateComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void USlidingStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool USlidingStateComponent::OnSetStateConditionCheck(UCharacterStateMachine& SM)
{
	return PlayerMovement->IsMovingOnGround();
}

void USlidingStateComponent::OnEnterState(UCharacterStateMachine& SM)
{
	Super::OnEnterState(SM);

	InternalTimer = 0;
	PlayerCharacter->bUseControllerRotationYaw = false;

	PlayerCharacter->LaunchCharacter(PlayerCharacter->GetActorForwardVector() * SlideSpeedCurve->GetFloatValue(0), false, false);


	CameraFullHeight = PlayerCharacter->GetFirstPersonCameraComponent()->GetRelativeLocation().Z;
	CameraReducedHeight = PlayerCharacter->GetFirstPersonCameraComponent()->GetRelativeLocation().Z / 4;

	FVector CamLoc = PlayerCharacter->GetFirstPersonCameraComponent()->GetRelativeLocation();
	CamLoc.Z = CameraReducedHeight;
	PlayerCharacter->GetFirstPersonCameraComponent()->SetRelativeLocation(CamLoc);
}

void USlidingStateComponent::OnUpdateState(UCharacterStateMachine& SM)
{
	Super::OnUpdateState(SM);


	if (InternalTimer <= MaxSlideDuration)
	{
		InternalTimer += GetWorld()->GetDeltaSeconds();
	}
	else
	{
		SM.ManualExitState();
	}
}

void USlidingStateComponent::OnExitState(UCharacterStateMachine& SM)
{
	Super::OnExitState(SM);
	PlayerCapsule->SetCapsuleSize(55.0f, 96.0f); //todo remove hard code.
	
	FVector CamLoc = PlayerCharacter->GetFirstPersonCameraComponent()->GetRelativeLocation();
	CamLoc.Z = CameraFullHeight;
	PlayerCharacter->GetFirstPersonCameraComponent()->SetRelativeLocation(CamLoc);

	PlayerCharacter->bUseControllerRotationYaw = true;


	if (PlayerMovement->Velocity.Length() >= PlayerCharacter->GetMaxRunningSpeed())
	{
		PlayerMovement->MaxWalkSpeed = PlayerCharacter->GetMaxRunningSpeed();
	}
}

void USlidingStateComponent::OverrideMovementInput(UCharacterStateMachine& SM, FVector2d& NewMovementVector)
{
	Super::OverrideMovementInput(SM, NewMovementVector);
	NewMovementVector.X *= SlidingLeftRightMovementModifier;
}

void USlidingStateComponent::OverrideCameraInput(UCharacterStateMachine& SM, FVector2d& NewRotationVector)
{
	Super::OverrideCameraInput(SM, NewRotationVector);
	if (OnlyModifyCameraLeftRight)
	{
		NewRotationVector.X *= SlidingCameraModifier;
	}
	else NewRotationVector *= SlidingCameraModifier;
}

void USlidingStateComponent::OverrideAcceleration(UCharacterStateMachine& SM, float& NewSpeed)
{
	Super::OverrideAcceleration(SM, NewSpeed);

	NewSpeed = SlideSpeedCurve->GetFloatValue(InternalTimer);
}
