// Fill out your copyright notice in the Description page of Project Settings.


#include "WallRunningStateComponent.h"

#include "GameFramework/CharacterMovementComponent.h"


// Sets default values for this component's properties
UWallRunningStateComponent::UWallRunningStateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	UActorComponent::SetComponentTickEnabled(false);


	// ...
}


// Called when the game starts
void UWallRunningStateComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UWallRunningStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UWallRunningStateComponent::OnSetStateConditionCheck(UCharacterStateMachine& SM)
{
	if (!LineTraceSingle(GetOwner()->GetActorLocation(), GetOwner()->GetActorLocation() - GetOwner()->GetActorUpVector() * MinimumDistanceFromGround)
		&& PrevResult.GetActor() != HitResult.GetActor() && PlayerCharacter->GetCharacterMovementInput().X != 0)
	{
		return true;
	}
	return false;
}


void UWallRunningStateComponent::OnEnterState(UCharacterStateMachine& SM)
{
	Super::OnEnterState(SM);
	PlayerCharacter->bUseControllerRotationYaw = false;
	GravityTimer = 0;
	WallRunTimer = 0.0f;
	InternalGravityScale = 0;
	PlayerMovement->Velocity.Z = 0;
	RotatePlayerAlongsideWall(HitResult);
}

void UWallRunningStateComponent::OnUpdateState(UCharacterStateMachine& SM)
{
	Super::OnUpdateState(SM);

	const FVector CounterGravityForce = FVector(0, 0, PlayerMovement->Mass * 980);
	const FVector GravityForce = FVector(0, 0, -PlayerMovement->Mass * 980) * WallRunGravityCurve->GetFloatValue(GravityTimer);

	PlayerMovement->AddForce(CounterGravityForce + GravityForce);
	WallRunTimer += GetWorld()->GetDeltaSeconds();

	if (WallRunTimer >= MaxWallRunDuration)
	{
		SM.ManualExitState();
	}
}

void UWallRunningStateComponent::OnExitState(UCharacterStateMachine& SM)
{
	Super::OnExitState(SM);
	PlayerCharacter->bUseControllerRotationYaw = true;
	PrevResult = HitResult;
	HitResult = EmptyResult;
}

void UWallRunningStateComponent::OverrideMovementInput(UCharacterStateMachine& SM, FVector2d& NewMovementVector)
{
	Super::OverrideMovementInput(SM, NewMovementVector);
	RotatePlayerAlongsideWall(HitResult);
	if (!CheckWhetherStillWallRunning()) SM.ManualExitState();
	NewMovementVector.X = 0;
}

void UWallRunningStateComponent::OverrideCameraInput(UCharacterStateMachine& SM, FVector2d& NewRotationVector)
{
	Super::OverrideCameraInput(SM, NewRotationVector);
}

void UWallRunningStateComponent::RotatePlayerAlongsideWall(const FHitResult& Hit) const
{
	FVector RotatedVector;

	if (WallOrientation == Left)
	{
		RotatedVector.X = Hit.Normal.Y;
		RotatedVector.Y = -Hit.Normal.X;
	}
	else if (WallOrientation == Right)
	{
		RotatedVector.X = -Hit.Normal.Y;
		RotatedVector.Y = Hit.Normal.X;
	}
	PlayerCharacter->SetActorRotation(RotatedVector.Rotation());

	const FVector NewVel = PlayerCharacter->GetActorForwardVector() * FVector(PlayerMovement->Velocity.X, PlayerMovement->Velocity.Y, 0).Size();
	PlayerMovement->Velocity = FVector(NewVel.X, NewVel.Y, PlayerMovement->Velocity.Z);
}

bool UWallRunningStateComponent::CheckWhetherStillWallRunning()
{
	//If moving on ground, cancel wall run outright, otherwise, check the side if we are still wall running
	if (PlayerMovement->IsMovingOnGround())
	{
		return false;
	}

	const FVector Start = PlayerCharacter->GetActorLocation();
	const int SideMultiplier = WallOrientation == Right ? 1 : -1; //return - 1 if it is left, to negate RightVector()
	const FVector End = Start + PlayerCharacter->GetActorRightVector() * WallCheckDistance * SideMultiplier * 3; //x3 to have a safer check at wall

	return LineTraceSingle(HitResult, Start, End);
}


void UWallRunningStateComponent::OverrideDebug()
{
	if (!DebugMechanic) return;

	Super::OverrideDebug();

	const FVector Start = GetOwner()->GetActorLocation();
	const FVector LengthVector = GetOwner()->GetActorRotation().Vector() * WallCheckDistance;

	//Side check debug. Right and Left
	DrawDebugLine(GetWorld(), Start, Start + RotateVector(LengthVector, FRotator(0, WallRunSideAngle, 0)), DebugColor, false, 0, 0, 3);
	DrawDebugLine(GetWorld(), Start, Start + RotateVector(LengthVector, FRotator(0, -WallRunSideAngle, 0)), DebugColor, false, 0, 0, 3);


	if (PlayerCharacter->GetCharacterStateMachine()->GetCurrentEnumState() == ECharacterState::WallRunning)
	{
		const int SideMultiplier = WallOrientation == Right ? 1 : -1; //return - 1 if it is left, to negate RightVector()
		//x3 to have a safer check at wall
		const FVector End = Start + PlayerCharacter->GetActorRightVector() * WallCheckDistance * SideMultiplier * 3;
		DrawDebugLine(GetWorld(), Start, End, DebugColor, false, 0, 0, 3);
	}
}

void UWallRunningStateComponent::OverrideDetectState(UCharacterStateMachine& SM)
{
	Super::OverrideDetectState(SM);

	if (PlayerMovement->IsMovingOnGround())
	{
		PrevResult = EmptyResult;
	}

	const FVector Start = GetOwner()->GetActorLocation();

	FHitResult RightSide;
	const FVector RightVector = RotateVector(GetOwner()->GetActorRotation().Vector(), FRotator(0, WallRunSideAngle, 0), WallCheckDistance);
	const bool bRightSideHit = LineTraceSingle(RightSide, Start, Start + RightVector);

	FHitResult LeftSide;
	const FVector LeftVector = RotateVector(GetOwner()->GetActorRotation().Vector(), FRotator(0, -WallRunSideAngle, 0), WallCheckDistance);
	const bool bLeftSideHit = LineTraceSingle(LeftSide, Start, Start + LeftVector);

	if (bRightSideHit && !bLeftSideHit)
	{
		WallOrientation = Right;
	}
	else if (!bRightSideHit && bLeftSideHit)
	{
		WallOrientation = Left;
	}
	else
	{
		WallOrientation = None;
	}

	if (WallOrientation != None)
	{
		const FHitResult NewResult = bRightSideHit ? RightSide : LeftSide;
		if (HitResult.GetActor() == NewResult.GetActor())
		{
			TriggerTimer += GetWorld()->GetDeltaSeconds();

			if (TriggerTimer >= WallRunTriggerDelay)
			{
				PlayerCharacter->GetCharacterStateMachine()->SetState(ECharacterState::WallRunning);
			}
		}
		else
		{
			HitResult = NewResult;
			TriggerTimer = 0;
		}
	}
	else
	{
		TriggerTimer = 0;
	}
}

void UWallRunningStateComponent::OverrideJump(UCharacterStateMachine& SM, FVector& JumpVector)
{
	Super::OverrideJump(SM, JumpVector);

	if (!DisableTapJump && FMath::Abs(PlayerCharacter->GetFirstPersonCameraComponent()->GetRelativeRotation().Yaw) < WallRunTapAngle)
	{
		FVector NewJumpVector;
		
		if (WallOrientation == Right)
		{
			NewJumpVector = -PlayerCharacter->GetActorRightVector();
		}
		else if (WallOrientation == Left)
		{
			NewJumpVector = PlayerCharacter->GetActorRightVector();
		}
		
		JumpVector = (NewJumpVector * TapJumpForceSideModifier + PlayerCharacter->GetActorUpVector() * TapJumpForceUpModifier) * JumpVector.Size();
	}
}

void UWallRunningStateComponent::OverrideNoMovementInputEvent(UCharacterStateMachine& SM)
{
	Super::OverrideNoMovementInputEvent(SM);
	SM.ManualExitState();
}
