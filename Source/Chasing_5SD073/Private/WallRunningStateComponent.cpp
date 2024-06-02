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
	CanExitState = true;


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
	if (!CloseToGround() && PlayerCharacter->GetLastInteractedWall() != HitResult.GetActor() /* && !NonInteractableWalls.Contains(HitResult.GetActor())*/
		&& (PlayerCharacter->GetCharacterMovementInput().X > 0.1f && WallOrientation == Right ||
			PlayerCharacter->GetCharacterMovementInput().X < -0.1f && WallOrientation == Left))

	{
		return true;
	}

	return false;
}


void UWallRunningStateComponent::OnEnterState(UCharacterStateMachine& SM)
{
	Super::OnEnterState(SM);
	NoLongerWallRunning = false;
	PlayerCharacter->bUseControllerRotationYaw = false;
	PlayerCharacter->SetLastInteractedWall(HitResult.GetActor());
	WallRunTimer = 0.0f;
	PlayerMovement->Velocity.Z = 0;
	RotatePlayerAlongsideWall(HitResult);
	PlayerForwardVectorOnEnter = PlayerCharacter->GetActorForwardVector();
	PlayerUpVectorOnEnter = PlayerCharacter->GetActorUpVector();
	TouchedGround = false;
	//BlockContinuousWall();
}

void UWallRunningStateComponent::OnUpdateState(UCharacterStateMachine& SM)
{
	Super::OnUpdateState(SM);

	const FVector CounterGravityForce = FVector(0, 0, PlayerMovement->Mass * 980);
	const FVector GravityForce = FVector(0, 0, -PlayerMovement->Mass * 980) * WallRunGravityCurve->GetFloatValue(WallRunTimer);

	PlayerMovement->AddForce(CounterGravityForce + GravityForce);

	if (!NoLongerWallRunning)
	{
		WallRunTimer += GetWorld()->GetDeltaSeconds();
	}

	if (WallRunTimer >= MaxWallRunDuration || PlayerMovement->IsMovingOnGround())
	{
		SM.ManualExitState();
	}
}

void UWallRunningStateComponent::OnExitState(UCharacterStateMachine& SM)
{
	Super::OnExitState(SM);
	PlayerCharacter->bUseControllerRotationYaw = true;
	LastPointOnWall = HitResult.ImpactPoint;
	PlayerCharacter->DisableJump();
}

void UWallRunningStateComponent::OverrideMovementInput(UCharacterStateMachine& SM, FVector2d& NewMovementVector)
{
	Super::OverrideMovementInput(SM, NewMovementVector);

	if (WallOrientation == Right && NewMovementVector.X <= 0)
	{
		SM.ManualExitState();
	}
	else if (WallOrientation == Left && NewMovementVector.X >= 0)
	{
		SM.ManualExitState();
	}

	if (!CheckWhetherStillWallRunning())
	{
		if (!NoLongerWallRunning)
		{
			NoLongerWallRunning = true;
			WallRunTimer = 0;
		}

		if (NoLongerWallRunning)
		{
			if (WallRunTimer <= PlayerCharacter->GetCoyoteTime())
			{
				WallRunTimer += GetWorld()->GetDeltaSeconds();
			}
			else
			{
				SM.ManualExitState();
				//return;
			}
		}
	}
	else RotatePlayerAlongsideWall(HitResult);

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

	if (LineTraceSingle(HitResult, Start, End))
	{
		if (HitResult.GetActor() != PlayerCharacter->GetLastInteractedWall())
		{
			PlayerCharacter->SetLastInteractedWall(HitResult.GetActor());
		}
		return true;
	}
	return false;
}

void UWallRunningStateComponent::BlockContinuousWall()
{
	NonInteractableWalls.Empty();

	bool NoGap = true;
	FHitResult MiddleBlockAnchor = HitResult;
	FHitResult UpBlockAnchor = HitResult;
	FHitResult DownBlockAnchor = HitResult;

	while (NoGap)
	{
		constexpr float SmallDistance = 2;

		const FBox BoundingBox = MiddleBlockAnchor.GetActor()->GetComponentsBoundingBox();

		float LongerSurfaceSize = 0;
		float ShorterSurfaceSize = 0;

		if (FMath::Abs(PlayerForwardVectorOnEnter.X) >= FMath::Abs(PlayerForwardVectorOnEnter.Y))
		{
			LongerSurfaceSize = BoundingBox.GetSize().X;
			ShorterSurfaceSize = BoundingBox.GetSize().Y;
		}
		else
		{
			LongerSurfaceSize = BoundingBox.GetSize().Y;
			ShorterSurfaceSize = BoundingBox.GetSize().X;
		}

		const FVector MiddlePoint = BoundingBox.GetCenter();

		//Middle point of the object on the surface
		const FVector NewMiddlePointIn = MiddlePoint + HitResult.ImpactNormal * (ShorterSurfaceSize / 2.0f);
		const FVector NewMiddlePointOut = MiddlePoint + HitResult.ImpactNormal * (ShorterSurfaceSize / 2.0f) * 1.1f;

		// Perform a line trace from the last point on the previous wall to a point just beyond the edge of the current wall
		FVector TraceEndIn = NewMiddlePointIn + PlayerForwardVectorOnEnter * (LongerSurfaceSize / 2.0f + SmallDistance);
		FVector TraceEndOut = NewMiddlePointOut + PlayerForwardVectorOnEnter * (LongerSurfaceSize / 2.0f + SmallDistance);

		FHitResult GapHitResultIn;
		FHitResult GapHitResultOut;

		FCollisionQueryParams TraceParams;
		TraceParams.AddIgnoredActor(GetOwner());
		TraceParams.AddIgnoredActor(MiddleBlockAnchor.GetActor()); // Ignore the origin actor

		//DrawDebugLine(GetWorld(), NewMiddlePointIn, TraceEndIn, FColor::Red, false, 20, 0, 10);
		DrawDebugLine(GetWorld(), NewMiddlePointOut, TraceEndOut, FColor::Red, false, 20, 0, 10);

		const bool InsideHit = GetWorld()->LineTraceSingleByChannel(GapHitResultIn, NewMiddlePointIn, TraceEndIn, ECC_Visibility, TraceParams);
		const bool OutsideHit = GetWorld()->LineTraceSingleByChannel(GapHitResultOut, NewMiddlePointOut, TraceEndOut, ECC_Visibility, TraceParams);

		if (InsideHit && !OutsideHit)
		{
			// It's a continuous wall, update LastUsedWall. We shouldn't be using this wall as it 'looks' like a single object.
			const FBox HitObj = GapHitResultIn.GetActor()->GetComponentsBoundingBox();
			DrawDebugBox(GetWorld(), HitObj.GetCenter(), HitObj.GetExtent(), FColor::Red, false, 20, 0, 20);
			MiddleBlockAnchor = GapHitResultIn;
			NonInteractableWalls.Add(MiddleBlockAnchor.GetActor());
		}
		else
		{
			NoGap = false;
			break;
		}

		bool UpNoGap = true;
		bool DownNoGap = true;


		while (UpNoGap) //going up
		{
			const FBox UpBoundingBox = UpBlockAnchor.GetActor()->GetComponentsBoundingBox();

			LongerSurfaceSize = UpBoundingBox.GetSize().Z;
			ShorterSurfaceSize = 0;
			
			if (FMath::Abs(PlayerForwardVectorOnEnter.X) >= FMath::Abs(PlayerForwardVectorOnEnter.Y))
			{
				//LongerSurfaceSize = UpBoundingBox.GetSize().X;
				ShorterSurfaceSize = UpBoundingBox.GetSize().Y;
			}
			else
			{
				//LongerSurfaceSize = UpBoundingBox.GetSize().Y;
				ShorterSurfaceSize = UpBoundingBox.GetSize().X;
			}
			
			const FVector UpMiddlePoint = UpBoundingBox.GetCenter();

			//Middle point of the object on the surface
			const FVector NewUpMiddlePointIn = UpMiddlePoint + HitResult.ImpactNormal * (ShorterSurfaceSize / 2.0f);
			const FVector NewUpMiddlePointOut = UpMiddlePoint + HitResult.ImpactNormal * (ShorterSurfaceSize / 2.0f) * 1.1f;

			const FVector TraceUpEndIn = NewUpMiddlePointIn + PlayerUpVectorOnEnter * (LongerSurfaceSize / 2.0f + SmallDistance);
			const FVector TraceUpEndOut = NewUpMiddlePointOut + PlayerUpVectorOnEnter * (LongerSurfaceSize / 2.0f + SmallDistance);

			TraceParams.AddIgnoredActor(UpBlockAnchor.GetActor()); // Ignore the origin actor

			FHitResult UpHitIn;
			FHitResult UpHitOut;

			const bool UpInsideHit = GetWorld()->LineTraceSingleByChannel(UpHitIn, NewUpMiddlePointIn, TraceUpEndIn, ECC_Visibility, TraceParams);
			const bool UpOutsideHit = GetWorld()->LineTraceSingleByChannel(UpHitOut, NewUpMiddlePointOut, TraceUpEndOut, ECC_Visibility, TraceParams);

			DrawDebugLine(GetWorld(), NewUpMiddlePointOut, TraceUpEndOut, FColor::Red, false, 20, 0, 10);

			if (UpInsideHit && !UpOutsideHit)
			{
				// It's a continuous wall, update LastUsedWall. We shouldn't be using this wall as it 'looks' like a single object.
				const FBox HitObj = UpHitIn.GetActor()->GetComponentsBoundingBox();
				DrawDebugBox(GetWorld(), HitObj.GetCenter(), HitObj.GetExtent(), FColor::Red, false, 20, 0, 20);
				UpBlockAnchor = UpHitIn;
				NonInteractableWalls.Add(UpBlockAnchor.GetActor());
			}
			else
			{
				UpNoGap = false;
				break;
			}
		}
		while (DownNoGap) //going down
		{
			const FBox DownBoundingBox = DownBlockAnchor.GetActor()->GetComponentsBoundingBox();

			LongerSurfaceSize = DownBoundingBox.GetSize().Z;
			ShorterSurfaceSize = 0;

			if (FMath::Abs(PlayerForwardVectorOnEnter.X) >= FMath::Abs(PlayerForwardVectorOnEnter.Y))
			{
				//LongerSurfaceSize = DownBoundingBox.GetSize().X;
				ShorterSurfaceSize = DownBoundingBox.GetSize().Y;
			}
			else
			{
				//LongerSurfaceSize = DownBoundingBox.GetSize().Y;
				ShorterSurfaceSize = DownBoundingBox.GetSize().X;
			}

			const FVector DownMiddlePoint = DownBoundingBox.GetCenter();

			//Middle point of the object on the surface
			const FVector NewDownMiddlePointIn = DownMiddlePoint + HitResult.ImpactNormal * (ShorterSurfaceSize / 2.0f);
			const FVector NewDownMiddlePointOut = DownMiddlePoint + HitResult.ImpactNormal * (ShorterSurfaceSize / 2.0f) * 1.1f;

			const FVector TraceDownEndIn = NewDownMiddlePointIn - PlayerUpVectorOnEnter * (LongerSurfaceSize / 2.0f + SmallDistance);
			const FVector TraceDownEndOut = NewDownMiddlePointOut - PlayerUpVectorOnEnter * (LongerSurfaceSize / 2.0f + SmallDistance);

			TraceParams.AddIgnoredActor(UpBlockAnchor.GetActor()); // Ignore the origin actor

			FHitResult DownHitIn;
			FHitResult DownHitOut;

			const bool DownInsideHit = GetWorld()->LineTraceSingleByChannel(DownHitIn, NewDownMiddlePointIn, TraceDownEndIn, ECC_Visibility, TraceParams);
			const bool DownOutsideHit = GetWorld()->LineTraceSingleByChannel(DownHitOut, NewDownMiddlePointOut, TraceDownEndOut, ECC_Visibility, TraceParams);
			
			DrawDebugLine(GetWorld(), NewDownMiddlePointOut, TraceDownEndOut, FColor::Red, false, 20, 0, 10);
			
			if (DownInsideHit && !DownOutsideHit)
			{
				// It's a continuous wall, update LastUsedWall. We shouldn't be using this wall as it 'looks' like a single object.
				//PlayerCharacter->SetLastInteractedWall(GapHitResultIn.GetActor());
				const FBox HitObj = DownHitIn.GetActor()->GetComponentsBoundingBox();
				DrawDebugBox(GetWorld(), HitObj.GetCenter(), HitObj.GetExtent(), FColor::Red, false, 20, 0, 20);
				DownBlockAnchor = DownHitIn;
				NonInteractableWalls.Add(DownBlockAnchor.GetActor());
			}
			else
			{
				DownNoGap = false;
				break;
			}
		}
	}
}

bool UWallRunningStateComponent::CloseToGround() const
{
	return LineTraceSingle(GetOwner()->GetActorLocation(),
	                       GetOwner()->GetActorLocation() - GetOwner()->GetActorUpVector() * MinimumDistanceFromGround);
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


	if (!TouchedGround && PlayerMovement->IsMovingOnGround())
	{
		TouchedGround = true;
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
		TriggerTimer = 0;
		WallOrientation = None;
		return;
	}


	if (WallOrientation != None)
	{
		//if (TouchedGround || HitResult.GetActor() == nullptr)
		//{
			const FHitResult NewResult = bRightSideHit ? RightSide : LeftSide;
			HitResult = NewResult;
			SM.SetState(ECharacterState::WallRunning);
		//}
		/*
		else
		{
			const FHitResult NewResult = bRightSideHit ? RightSide : LeftSide;
			constexpr float SmallDistance = 1;

			const FBox BoundingBox = HitResult.GetActor()->GetComponentsBoundingBox();

			float LongerSurfaceSize = 0;
			float ShorterSurfaceSize = 0;

			if (FMath::Abs(PlayerForwardVectorOnEnter.X) >= FMath::Abs(PlayerForwardVectorOnEnter.Y))
			{
				LongerSurfaceSize = BoundingBox.GetSize().X;
				ShorterSurfaceSize = BoundingBox.GetSize().Y;
			}
			else
			{
				LongerSurfaceSize = BoundingBox.GetSize().Y;
				ShorterSurfaceSize = BoundingBox.GetSize().X;
			}

			const FVector MiddlePoint = BoundingBox.GetCenter();

			//Middle point of the object on the surface
			const FVector NewMiddlePointIn = MiddlePoint + HitResult.ImpactNormal * (ShorterSurfaceSize / 2.0f) * 0.9f;
			const FVector NewMiddlePointOut = MiddlePoint + HitResult.ImpactNormal * (ShorterSurfaceSize / 2.0f) * 1.1f;

			// Perform a line trace from the last point on the previous wall to a point just beyond the edge of the current wall
			FVector TraceEndIn = NewMiddlePointIn + PlayerForwardVectorOnEnter * (LongerSurfaceSize / 2.0f + SmallDistance);
			FVector TraceEndOut = NewMiddlePointOut + PlayerForwardVectorOnEnter * (LongerSurfaceSize / 2.0f + SmallDistance);

			FHitResult GapHitResultIn;
			FHitResult GapHitResultOut;

			FCollisionQueryParams TraceParams;
			TraceParams.AddIgnoredActor(GetOwner());
			TraceParams.AddIgnoredActor(HitResult.GetActor()); // Ignore the origin actor

			DrawDebugLine(GetWorld(), NewMiddlePointIn, TraceEndIn, FColor::Red, false, 20, 0, 10);
			DrawDebugLine(GetWorld(), NewMiddlePointOut, TraceEndOut, FColor::Red, false, 20, 0, 10);

			const bool InsideHit = GetWorld()->LineTraceSingleByChannel(GapHitResultIn, NewMiddlePointIn, TraceEndIn, ECC_Visibility, TraceParams);
			const bool OutsideHit = GetWorld()->
				LineTraceSingleByChannel(GapHitResultOut, NewMiddlePointOut, TraceEndOut, ECC_Visibility, TraceParams);

			if (InsideHit && !OutsideHit)
			{
				// It's a continuous wall, update LastUsedWall. We shouldn't be using this wall as it 'looks' like a single object.
				PlayerCharacter->SetLastInteractedWall(GapHitResultIn.GetActor());
				const FBox HitObj = GapHitResultIn.GetActor()->GetComponentsBoundingBox();
				DrawDebugBox(GetWorld(), HitObj.GetCenter(), HitObj.GetExtent(), FColor::Red, false, 20, 0, 10);
			}
			else //It either didn't anything, so there's a gap. Or, There is an indent, blocking continuous wall
			{
				HitResult = NewResult;
				SM.SetState(ECharacterState::WallRunning);
			}
		}
		*/
	}
}

void UWallRunningStateComponent::OverrideJump(UCharacterStateMachine& SM, FVector& JumpVector)
{
	Super::OverrideJump(SM, JumpVector);

	/*
	if (FMath::Abs(PlayerCharacter->GetFirstPersonCameraComponent()->GetRelativeRotation().Yaw) < WallRunTapAngle)
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

		CanExitState = false;
		JumpVector = FVector::ZeroVector;
	}
	else CanExitState = true;
	*/
}

void UWallRunningStateComponent::OverrideNoMovementInputEvent(UCharacterStateMachine& SM)
{
	Super::OverrideNoMovementInputEvent(SM);
	SM.ManualExitState();
}
