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

void USlidingStateComponent::OnEnterState(UCharacterStateMachine& SM)
{
	Super::OnEnterState(SM);
	
	InternalTimerStart = FApp::GetCurrentTime();
	PlayerCharacter->bUseControllerRotationYaw = false;
	PlayerCharacter->bUseControllerRotationPitch = false;
	
	
}

void USlidingStateComponent::OnUpdateState(UCharacterStateMachine& SM)
{
	Super::OnUpdateState(SM);

	

	if (FApp::GetCurrentTime() - InternalTimerStart < MaxSlideDuration)
	{
		//PlayerCapsule->SetCapsuleHalfHeight(PlayerCapsule->GetScaledCapsuleHalfHeight() / 2);
		PlayerCharacter->AddCharacterSpeed(SlidingSpeedBoost);
		//slide boost, thought might change this if its an acceleration rather than static boost
	}
	else
	{
		SM.ResetState();
	}
}

void USlidingStateComponent::OnExitState(UCharacterStateMachine& SM)
{
	Super::OnExitState(SM);
	//PlayerCapsule->SetCapsuleSize(55.0f, 96.0f);

	if (UsedCamera != nullptr)
	{
		FVector CamLoc = UsedCamera->GetRelativeLocation();
		CamLoc.Z = CameraFullHeight;
		UsedCamera->SetRelativeLocation(CamLoc);
	}

	PlayerCharacter->bUseControllerRotationYaw = true;
	PlayerCharacter->bUseControllerRotationPitch = true;

	if (PlayerMovement->Velocity.Length() >= 1000)
	{
		PlayerMovement->MaxWalkSpeed = 1000; //rmeove hardcode later.
	}

	IsSlidingSetup = false;
}

void USlidingStateComponent::OverrideMovementInputSensitivity(FVector2d& NewMovementVector)
{
	NewMovementVector.X *= SlidingLeftRightMovementModifier;
}

void USlidingStateComponent::OverrideCamera(UCameraComponent& Camera, FVector2d& NewRotationVector)
{
	if (!IsSlidingSetup)
	{
		UsedCamera = &Camera;
		CameraFullHeight = Camera.GetRelativeLocation().Z;
		CameraReducedHeight = Camera.GetRelativeLocation().Z / 4;
		
		FVector CamLoc = Camera.GetRelativeLocation();
		CamLoc.Z = CameraReducedHeight;
		Camera.SetRelativeLocation(CamLoc);
		IsSlidingSetup = true;
	}

	NewRotationVector *= SlidingCameraModifier; 
}