// Fill out your copyright notice in the Description page of Project Settings.


#include "SlidingStateComponent.h"

#include "GameFramework/CharacterMovementComponent.h"

// Sets default values for this component's properties
USlidingStateComponent::USlidingStateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

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
	
	if (IsCapsuleShrunk == true)
	{
		FVector SweepStart = PlayerCapsule->GetComponentLocation() + FVector::UpVector * 50.0f;
		FVector SweepEnd = SweepStart + FVector::UpVector * 10.0f;
		
		if (SweepSingle(SweepStart, SweepEnd))
		{
			UE_LOG(LogTemp, Display, TEXT("Stuck"));
			return;
		}

		UE_LOG(LogTemp, Display, TEXT("Un Stuck"));
		
		PlayerCapsule->SetCapsuleSize(55.0f, 96.0f); //todo remove hard code.
		IsCapsuleShrunk = false;
	}
}

bool USlidingStateComponent::OnSetStateConditionCheck(UCharacterStateMachine& SM)
{
	if (PlayerCharacter->GetCharacterMovementState() != EMovementState::Idle && DetectGround())
	{
		return true;
	}
	return false;
}



void USlidingStateComponent::OnEnterState(UCharacterStateMachine& SM)
{
	Super::OnEnterState(SM);

	InternalTimer = 0;
	PlayerCharacter->bUseControllerRotationYaw = false;

	const float Ratio = PlayerCharacter->GetHorizontalVelocity() / PlayerCharacter->GetMaxRunningSpeed();
	PlayerCharacter->LaunchCharacter(PlayerCharacter->GetActorForwardVector() * SlideSpeedCurve->GetFloatValue(0) * Ratio, false, false);

	CameraFullHeight = PlayerCharacter->GetFirstPersonCameraComponent()->GetRelativeLocation().Z;
	CameraReducedHeight = PlayerCharacter->GetFirstPersonCameraComponent()->GetRelativeLocation().Z / 4;
	FVector CamLoc = PlayerCharacter->GetFirstPersonCameraComponent()->GetRelativeLocation();
	CamLoc.Z = CameraReducedHeight;
	PlayerCharacter->GetFirstPersonCameraComponent()->SetRelativeLocation(CamLoc);

	PlayerCapsule->SetCapsuleSize(55.0f, 55.0f); //todo remove hard code.
}

void USlidingStateComponent::OnUpdateState(UCharacterStateMachine& SM)
{
	Super::OnUpdateState(SM);

	if (InternalTimer <= MaxSlideDuration)
	{
		InternalTimer += GetWorld()->GetDeltaSeconds();
	}
	else SM.ManualExitState();
}

void USlidingStateComponent::OnExitState(UCharacterStateMachine& SM)
{
	Super::OnExitState(SM);
	if (!LineTraceSingle(PlayerCapsule->GetComponentLocation(), PlayerCapsule->GetComponentLocation() * GetOwner()->GetActorUpVector() * 100))
	{
		PlayerCapsule->SetCapsuleSize(55.0f, 96.0f); //todo remove hard code.
	}
	else
	{
		IsCapsuleShrunk = true;
	}

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

bool USlidingStateComponent::DetectGround() const
{
	const FVector Start = GetOwner()->GetActorLocation();
	const float FallingMultiplier = PlayerMovement->Velocity.Z < -PlayerCharacter->GetMaxRunningSpeed() ? FMath::Abs(PlayerMovement->Velocity.Z) / PlayerCharacter->GetMaxRunningSpeed() : 1;
	return LineTraceSingle(Start, Start - GetOwner()->GetActorUpVector() * AboutToFallDetectionDistance * FallingMultiplier);
}

bool USlidingStateComponent::SweepSingle(FVector& Start, FVector& End) const
{
	FHitResult HitR;
	FHitResult HitR2;
	const FVector Offset = FVector(0,0, -PlayerCapsule->GetScaledCapsuleHalfHeight());
	const auto LineStart = PlayerCharacter->GetActorLocation() + Offset;
	const auto LineEnd = PlayerCharacter->GetActorLocation() - GetOwner()->GetActorUpVector() * 50.f + Offset;
	LineTraceSingle(HitR2, LineStart, LineEnd);
	DrawDebugLine(GetWorld(), LineStart, LineEnd, FColor::Green, false, 0, 0, 3);
	FCollisionQueryParams CollisionParams;
	
	CollisionParams.AddIgnoredActor(GetOwner());
	//CollisionParams.AddIgnoredActor(HitR2.GetActor());
	
	const FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(55.0f, 96.0f);
	DrawDebugCapsule(GetWorld(), Start, 55.0f, 96.0f, FQuat::Identity, FColor::Green, false, 0, 0, 3);
	return GetWorld()->SweepSingleByChannel(HitR, Start, End, FQuat::Identity, ECC_Visibility, CollisionShape, CollisionParams);
}

void USlidingStateComponent::OverrideDebug()
{
	if (!DebugMechanic) return;
	
	Super::OverrideDebug();
	const FVector Start = GetOwner()->GetActorLocation();
	DrawDebugLine(GetWorld(), Start, Start - GetOwner()->GetActorUpVector() * AboutToFallDetectionDistance, DebugColor, false, 0, 0, 3);
}
