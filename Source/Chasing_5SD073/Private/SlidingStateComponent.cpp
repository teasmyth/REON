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
		
		if (SweepCapsuleSingle(SweepStart, SweepEnd)) return;

		GetWorld()->GetTimerManager().SetTimer(CapsuleSizeResetTimer, this, &USlidingStateComponent::ResetCapsuleSize, 0.01f, true);
		//PlayerCapsule->SetCapsuleSize(55.0f, 96.0f); //todo remove hard code.
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

	if (!IsOnSlope())
	{
		const float Ratio = PlayerCharacter->GetHorizontalVelocity() / PlayerCharacter->GetMaxRunningSpeed();
		PlayerCharacter->LaunchCharacter(PlayerCharacter->GetActorForwardVector() * SlideSpeedCurve->GetFloatValue(0) * Ratio, false, false);
	}
	
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
	
	if (!IsOnSlope())
	{
		if (InternalTimer <= MaxSlideDuration)
		{
			InternalTimer += GetWorld()->GetDeltaSeconds();
		}
		else SM.ManualExitState();
	}
}

void USlidingStateComponent::OnExitState(UCharacterStateMachine& SM)
{
	Super::OnExitState(SM);
	if (!LineTraceSingle(PlayerCapsule->GetComponentLocation(), PlayerCapsule->GetComponentLocation() * GetOwner()->GetActorUpVector() * 100))
	{
		GetWorld()->GetTimerManager()
			.SetTimer(CapsuleSizeResetTimer, this, &USlidingStateComponent::ResetCapsuleSize, 0.01f, true);
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

static float Time = 0.0f;

void USlidingStateComponent::ResetCapsuleSize()
{
	if (Time <= CapsuleResizeDuration)
	{
		PlayerCapsule->SetCapsuleSize(55.0f, FMath::Lerp(55.0f, 96.0f, Time / CapsuleResizeDuration));
		Time += GetWorld()->GetDeltaSeconds();
	}
	else
	{
		Time = 0.0f;
		GetWorld()->GetTimerManager().ClearTimer(CapsuleSizeResetTimer);
	}
}

bool USlidingStateComponent::SweepCapsuleSingle(FVector& Start, FVector& End) const
{
	FHitResult HitR, HitR2;

	const FVector Offset = FVector(0,0, -PlayerCapsule->GetScaledCapsuleHalfHeight());
	const auto LineStart = PlayerCharacter->GetActorLocation() + Offset;
	const auto LineEnd = PlayerCharacter->GetActorLocation() - GetOwner()->GetActorUpVector() * 50.f + Offset;
	
	LineTraceSingle(HitR2, LineStart, LineEnd);
	//DrawDebugLine(GetWorld(), LineStart, LineEnd, FColor::Green, false, 0, 0, 3);
	//DrawDebugCapsule(GetWorld(), Start, 55.0f, 96.0f, FQuat::Identity, FColor::Green, false, 0, 0, 3);

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(GetOwner());
	//CollisionParams.AddIgnoredActor(HitR2.GetActor());
	
	const FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(55.0f, 96.0f);
	
	return GetWorld()->SweepSingleByChannel(HitR, Start, End, FQuat::Identity, ECC_Visibility, CollisionShape, CollisionParams);
}

bool USlidingStateComponent::IsOnSlope() const
{
	const FVector Offset = FVector(0,0, -PlayerCapsule->GetScaledCapsuleHalfHeight());
	const auto Start = PlayerCharacter->GetActorLocation() + Offset;
	const auto End = PlayerCharacter->GetActorLocation() - GetOwner()->GetActorUpVector() * 10.0f + Offset;
	bool OnSlope = false;
	float DotProduct = 0.0f;
	
	if (FHitResult HitResult; LineTraceSingle(HitResult, Start, End))
	{
		float Angle = 0.0f;
		Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(HitResult.ImpactNormal, FVector::UpVector)));
		OnSlope = FMath::Abs(Angle) > 1e-6;
		DotProduct = FVector::DotProduct(PlayerCharacter->GetVelocity(), HitResult.ImpactNormal);
	}

	return OnSlope && DotProduct > 0.0f;
}

void USlidingStateComponent::OverrideDebug()
{
	if (!DebugMechanic) return;
	
	Super::OverrideDebug();
	const FVector Start = GetOwner()->GetActorLocation();
	DrawDebugLine(GetWorld(), Start, Start - GetOwner()->GetActorUpVector() * AboutToFallDetectionDistance, DebugColor, false, 0, 0, 3);
}
