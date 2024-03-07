// Fill out your copyright notice in the Description page of Project Settings.


#include "WallClimbingStateComponent.h"

#include "AssetRegistry/Private/AssetRegistryImpl.h"

// Sets default values for this component's properties
UWallClimbingStateComponent::UWallClimbingStateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

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
}

bool UWallClimbingStateComponent::OnSetStateConditionCheck(UCharacterStateMachine& SM)
{
	if (PrevResult.GetActor() != HitResult.GetActor() && PlayerCharacter->GetCharacterMovementInput().Y > 0.01f)
	{
		return true;
	}
	return false;
}

void UWallClimbingStateComponent::OnEnterState(UCharacterStateMachine& SM)
{
	Super::OnEnterState(SM);
	PlayerCharacter->bUseControllerRotationYaw = false;
	InternalTimer = 0;
	PlayerMovement->Velocity = FVector(0, 0, PlayerMovement->Velocity.Z / 4.0f);
	PlayerMovement->UpdateComponentVelocity();
	PlayerCharacter->SetActorRotation((-HitResult.Normal).Rotation());
}

void UWallClimbingStateComponent::OnUpdateState(UCharacterStateMachine& SM)
{
	Super::OnUpdateState(SM);

	PlayerMovement->AddForce(FVector(0, 0, PlayerMovement->Mass * 980));
	InternalTimer += GetWorld()->GetDeltaSeconds();

	if (InternalTimer >= MaxWallClimbDuration || CheckLedge() && CheckLeg())
	{
		SM.ManualExitState();
	}
}

void UWallClimbingStateComponent::OnExitState(UCharacterStateMachine& SM)
{
	Super::OnExitState(SM);
	PlayerCharacter->bUseControllerRotationYaw = true;
	PrevResult = HitResult;
}

void UWallClimbingStateComponent::OverrideMovementInput(UCharacterStateMachine& SM, FVector2d& NewMovementVector)
{
	Super::OverrideMovementInput(SM, NewMovementVector);

	if (NewMovementVector.Y < 0.01f)
	{
		SM.ManualExitState();
	}

	const float CurrentSpeed = WallClimbSpeed * GetWorld()->GetDeltaSeconds();
	const float VerticalMovement = CurrentSpeed * (InternalTimer / MaxWallClimbDuration);
	PlayerCharacter->SetActorLocation(PlayerCharacter->GetActorLocation() + FVector(0.0f, 0.0f, VerticalMovement), true);
	NewMovementVector = FVector2d::ZeroVector;
}

void UWallClimbingStateComponent::OverrideNoMovementInputEvent(UCharacterStateMachine& SM)
{
	Super::OverrideNoMovementInputEvent(SM);

	SM.ManualExitState();
}


bool UWallClimbingStateComponent::CheckLedge() const
{
	const FVector Start = PlayerCapsule->GetComponentLocation() + FVector(0, 0, LedgeGrabCheckZOffset);
	const FVector End = Start + PlayerCharacter->GetActorForwardVector() * LedgeGrabCheckDistance;

	if (!LineTraceSingle(Start, End))
	{
		return true;
	}
	return false;
}

bool UWallClimbingStateComponent::CheckLeg() const
{
	const FVector Start = PlayerCapsule->GetComponentLocation() - FVector(0, 0, PlayerCapsule->GetScaledCapsuleHalfHeight());
	const FVector End = Start + PlayerCharacter->GetActorForwardVector() * LedgeGrabCheckDistance;

	if (!LineTraceSingle(Start, End))
	{
		return true;
	}
	return false;
}

void UWallClimbingStateComponent::DetectWallClimb()
{
	const FVector Start = GetOwner()->GetActorLocation();
	const FVector End = Start + GetOwner()->GetActorRotation().Vector() * WallCheckDistance;

	//Prioritizing player's aim.
	if (LineTraceSingle(HitResult, Start, End))
	{
		// Calculate the angle of incidence
		const float AngleInDegrees =
			FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(HitResult.Normal, -GetOwner()->GetActorForwardVector())));

		// AngleInDegrees now contains the angle between the wall and the actor's forward vector
		if (AngleInDegrees <= WallClimbAngle)
		{
			TriggerTimer += GetWorld()->GetDeltaSeconds();

			if (TriggerTimer >= WallClimbTriggerDelay)
			{
				PlayerCharacter->GetCharacterStateMachine()->SetState(ECharacterState::WallClimbing);
			}
		}
		else
		{
			TriggerTimer = 0;
		}
	}
	else TriggerTimer = 0;
}

void UWallClimbingStateComponent::OverrideDebug()
{
	if (!DebugMechanic) return;

	//Super::OverrideDebug();

	const FVector Start = GetOwner()->GetActorLocation();
	const FVector End = PlayerCharacter->GetActorForwardVector() * LedgeGrabCheckDistance;

	//Wall Climb Detection
	const FVector ForwardVec = GetOwner()->GetActorForwardVector() * WallCheckDistance;
	DrawDebugLine(GetWorld(), Start, Start + ForwardVec, DebugColor, false, 0, 0, 3);
	DrawDebugLine(GetWorld(), Start, Start + RotateVector(ForwardVec, FRotator(0, WallClimbAngle, 0)), DebugColor, false, 0, 0, 3);
	DrawDebugLine(GetWorld(), Start, Start + RotateVector(ForwardVec, FRotator(0, -WallClimbAngle, 0)), DebugColor, false, 0, 0, 3);

	//Ledge Detection
	const FVector LedgeStart = Start + FVector(0, 0, LedgeGrabCheckZOffset);
	DrawDebugLine(GetWorld(), LedgeStart, LedgeStart + End, DebugColor, false, 0, 0, 3);

	//Leg Detection
	const FVector LegStart = Start - FVector(0, 0, PlayerCapsule->GetScaledCapsuleHalfHeight());
	DrawDebugLine(GetWorld(), LegStart, LegStart + End, DebugColor, false, 0, 0, 3);
}

void UWallClimbingStateComponent::OverrideDetectState(UCharacterStateMachine& SM)
{
	Super::OverrideDetectState(SM);

	if (PlayerMovement->IsMovingOnGround()) PrevResult = EmptyResult;

	const FVector Start = GetOwner()->GetActorLocation();
	const FVector End = Start + GetOwner()->GetActorRotation().Vector() * WallCheckDistance;

	//Prioritizing player's aim.
	if (LineTraceSingle(HitResult, Start, End))
	{
		// Calculate the angle of incidence
		const float AngleInDegrees =
			FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(HitResult.Normal, -GetOwner()->GetActorForwardVector())));

		// AngleInDegrees now contains the angle between the wall and the actor's forward vector
		if (AngleInDegrees <= WallClimbAngle)
		{
			TriggerTimer += GetWorld()->GetDeltaSeconds();

			if (TriggerTimer >= WallClimbTriggerDelay)
			{
				PlayerCharacter->GetCharacterStateMachine()->SetState(ECharacterState::WallClimbing);
			}
		}
		else
		{
			TriggerTimer = 0;
		}
	}
	else TriggerTimer = 0;
}

void UWallClimbingStateComponent::OverrideJump(UCharacterStateMachine& SM, FVector& JumpVector)
{
	Super::OverrideJump(SM, JumpVector);

	if (FMath::Abs(PlayerCharacter->GetFirstPersonCameraComponent()->GetRelativeRotation().Yaw) < WallClimbAngle)
	{
		JumpVector = (PlayerCharacter->GetActorUpVector() * WallFacingJumpUpForceMultiplier - PlayerCharacter->GetActorForwardVector() *
			WallFacingJumpBackForceMultiplier) * JumpVector.Size();

		DrawDebugLine(GetWorld(), PlayerCharacter->GetActorLocation(), PlayerCharacter->GetActorLocation() + JumpVector, FColor::Red, false, 4, 0, 3);
	}
}
