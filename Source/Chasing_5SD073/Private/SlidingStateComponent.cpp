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
	if (PlayerCharacter->GetCharacterMovementState() != EMovementState::Idle && DetectGround() && !IsTimerOn(CapsuleSizeResetTimer))
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

	if (FSurfaceInfo Info; !IsOnSlope(Info))
	{
		const float Ratio = PlayerCharacter->GetHorizontalVelocity() / PlayerCharacter->GetMaxRunningSpeed();
		PlayerCharacter->LaunchCharacter(PlayerCharacter->GetActorForwardVector() * SlideSpeedCurve->GetFloatValue(0) * Ratio, false, false);
	}
	
	CameraFullHeight = PlayerCharacter->GetFirstPersonCameraComponent()->GetRelativeLocation().Z;
	CameraReducedHeight = PlayerCharacter->GetFirstPersonCameraComponent()->GetRelativeLocation().Z / 4;

	CapsuleCurrentHeight = PlayerCapsule->GetScaledCapsuleHalfHeight();
	CameraCurrentHeight = PlayerCharacter->GetFirstPersonCameraComponent()->GetRelativeLocation().Z;
	
	if (IsTimerOn(CapsuleSizeResetTimer))
		StopTimer(CapsuleSizeResetTimer);
	GetWorld()->GetTimerManager().SetTimer(CapsuleSizeShrinkTimer, this, &USlidingStateComponent::ShrinkCapsuleSize, 0.01f, true);
}

static bool WasUnderObject = false;

void USlidingStateComponent::OnUpdateState(UCharacterStateMachine& SM)
{
	Super::OnUpdateState(SM);
	
	if (IsUnderObject())
	{
		CanExitState = false;
	}
	else
	{
		CanExitState = true;
	}

	const auto Info = GetSurfaceInfo();
	if (!Info.IsSlopedSurface || (Info.IsSlopedSurface && Info.MovingDown))
	{
		if (InternalTimer > MaxSlideDuration || !IsUnderObject() && WasUnderObject)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Slide Time Over");
			SM.ManualExitState();
		}
		else
		{
			InternalTimer += GetWorld()->GetDeltaSeconds();
		}
	}

	if (IsUnderObject())
	{
		WasUnderObject = true;
	}
	else
	{
		WasUnderObject = false;
	}
}

void USlidingStateComponent::OnExitState(UCharacterStateMachine& SM)
{
	Super::OnExitState(SM);

	WasUnderObject = false;
	
	CapsuleCurrentHeight = PlayerCapsule->GetScaledCapsuleHalfHeight();
	CameraCurrentHeight = PlayerCharacter->GetFirstPersonCameraComponent()->GetRelativeLocation().Z;

	if (IsTimerOn(CapsuleSizeShrinkTimer))
		StopTimer(CapsuleSizeShrinkTimer);
	
	GetWorld()->GetTimerManager()
				  .SetTimer(CapsuleSizeResetTimer, this, &USlidingStateComponent::ResetCapsuleSize, 0.01f, true);
	
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

void USlidingStateComponent::ResetCapsuleSize()
{
	if (ExpandTime <= CapsuleResizeDuration)
	{
		const float t = CapsuleSizeCurve->GetFloatValue(ExpandTime / CapsuleResizeDuration);
		PlayerCapsule->SetCapsuleSize(55.0f, FMath::Lerp(CapsuleCurrentHeight, CapsuleMaxHeight, t));
		
		FVector CamLoc = PlayerCharacter->GetFirstPersonCameraComponent()->GetRelativeLocation();
		CamLoc.Z = FMath::Lerp(CameraCurrentHeight, CameraMaxHeight, t);
		PlayerCharacter->GetFirstPersonCameraComponent()->SetRelativeLocation(CamLoc);
		
		ExpandTime += GetWorld()->GetDeltaSeconds();
	}
	else
	{
		ExpandTime = 0.0f;
		GetWorld()->GetTimerManager().ClearTimer(CapsuleSizeResetTimer);
	}
}

void USlidingStateComponent::ShrinkCapsuleSize()
{
	if (ShrinkTime <= CapsuleShrinkDuration)
	{
		const float t = CapsuleSizeCurve->GetFloatValue(ShrinkTime / CapsuleShrinkDuration);
		PlayerCapsule->SetCapsuleSize(55.0f, FMath::Lerp(CapsuleCurrentHeight, CapsuleMinHeight, t));
		
		FVector CamLoc = PlayerCharacter->GetFirstPersonCameraComponent()->GetRelativeLocation();
		CamLoc.Z = FMath::Lerp(CameraCurrentHeight, CameraMinHeight, t);
		PlayerCharacter->GetFirstPersonCameraComponent()->SetRelativeLocation(CamLoc);
		
		ShrinkTime += GetWorld()->GetDeltaSeconds();
	}
	else
	{
		ShrinkTime = 0.0f;
		GetWorld()->GetTimerManager().ClearTimer(CapsuleSizeShrinkTimer);
	}
}

bool USlidingStateComponent::IsUnderObject() const
{
	FVector SweepStart = PlayerCharacter->GetActorLocation() + FVector::UpVector * 50.0f - FVector::ForwardVector * 1.0f;
	FVector SweepEnd = SweepStart + FVector::UpVector * 10.0f;
		
	return SweepCapsuleSingle(SweepStart, SweepEnd);
}

bool USlidingStateComponent::SweepCapsuleSingle(FVector& Start, FVector& End) const
{
	FHitResult HitR, HitR2;

	const FVector Offset = FVector(0,0, -PlayerCapsule->GetScaledCapsuleHalfHeight());
	const auto LineStart = PlayerCharacter->GetActorLocation() + Offset;
	const auto LineEnd = PlayerCharacter->GetActorLocation() - GetOwner()->GetActorUpVector() * 50.f + Offset;
	
	LineTraceSingle(HitR2, LineStart, LineEnd);

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(GetOwner());

	const FCollisionShape CollisionShape = FCollisionShape::MakeBox(FVector(55.0f, 75.0f, 96.0f));
	FQuat Rotation = FQuat::Identity;
	
	if (FSurfaceInfo Info; IsOnSlope(Info))
	{
		Rotation = FQuat(FRotator(Info.Angle, 0, 0));
	}
	
	return GetWorld()->SweepSingleByChannel(HitR, Start, End, Rotation, ECC_Visibility, CollisionShape, CollisionParams);
}

bool USlidingStateComponent::IsOnSlope(FSurfaceInfo& SlopeInfo) const
{
	const FVector Offset = FVector(0,0, -PlayerCapsule->GetScaledCapsuleHalfHeight());
	const auto Start = PlayerCharacter->GetActorLocation() + Offset;
	const auto End = PlayerCharacter->GetActorLocation() - GetOwner()->GetActorUpVector() * 10.0f + Offset;
	
	bool OnSlope = false;
	float DotProduct = 0.0f;
	float Angle = 0.0f;
	
	if (FHitResult HitResult; LineTraceSingle(HitResult, Start, End))
	{
		// Angle between the normal of the hit and the up vector
		Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(HitResult.ImpactNormal, FVector::UpVector)));
		// If the angle is not 0, then the player is on a slope
		OnSlope = FMath::Abs(Angle) > 1e-6;
		// Dot product between the player's velocity and the normal of the hit
		DotProduct = FVector::DotProduct(PlayerCharacter->GetVelocity(), HitResult.ImpactNormal);

		SlopeInfo.Angle = Angle;
		SlopeInfo.MovingDown = DotProduct > 0.0f;
		SlopeInfo.Normal = HitResult.ImpactNormal;
	}
	
	return OnSlope;
}

FSurfaceInfo USlidingStateComponent::GetSurfaceInfo() const
{
	FSurfaceInfo Info;
	
	const FVector Offset = FVector(0,0, -PlayerCapsule->GetScaledCapsuleHalfHeight());
	const auto Start = PlayerCharacter->GetActorLocation() + Offset;
	const auto End = PlayerCharacter->GetActorLocation() - GetOwner()->GetActorUpVector() * 10.0f + Offset;
	
	bool OnSlope = false;
	float DotProduct = 0.0f;
	float Angle = 0.0f;
	
	if (FHitResult HitResult; LineTraceSingle(HitResult, Start, End))
	{
		// Angle between the normal of the hit and the up vector
		Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(HitResult.ImpactNormal, FVector::UpVector)));
		// If the angle is not 0, then the player is on a slope
		OnSlope = FMath::Abs(Angle) > 1e-6;
		// Dot product between the player's velocity and the normal of the hit
		DotProduct = FVector::DotProduct(PlayerCharacter->GetVelocity(), HitResult.ImpactNormal);

		Info.Angle = Angle;
		Info.MovingDown = DotProduct < 0.0f;
		Info.Normal = HitResult.ImpactNormal;
		Info.IsSlopedSurface = OnSlope;
	}
	
	return Info;
}

bool USlidingStateComponent::IsTimerOn(const FTimerHandle& Timer) const
{
	return GetWorld()->GetTimerManager().IsTimerActive(Timer);
}

void USlidingStateComponent::StopTimer(FTimerHandle& Timer) const
{
	GetWorld()->GetTimerManager().ClearTimer(Timer);
}

void USlidingStateComponent::OverrideDebug()
{
	if (!DebugMechanic) return;
	
	Super::OverrideDebug();
	const FVector Start = GetOwner()->GetActorLocation();
	DrawDebugLine(GetWorld(), Start, Start - GetOwner()->GetActorUpVector() * AboutToFallDetectionDistance, DebugColor, false, 0, 0, 3);
}
