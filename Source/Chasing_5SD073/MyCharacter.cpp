// Fill out your copyright notice in the Description page of Project Settings.

#include "MyCharacter.h"
#include "CharacterStateMachine.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "StateComponentBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

#pragma region Character
// Sets default values
AMyCharacter::AMyCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// Create a CameraComponent	
	FrontCam = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FrontCam->SetupAttachment(GetCapsuleComponent());
	FrontCam->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FrontCam->bUsePawnControlRotation = true;
	FrontCam->Activate(true);

	BackCam = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Cam"));
	BackCam->SetupAttachment(GetCapsuleComponent());
	BackCam->Activate(false);

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FrontCam);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));
}

// Called when the game starts or when spawned
void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	StateMachine = GetComponentByClass<UCharacterStateMachine>();
	StateMachine->SetupStateMachine();
	StateMachine->SetState(ECharacterState::DefaultState);
	GetCharacterMovement()->MaxCustomMovementSpeed = MaxRunningSpeed;
	FallStartZ = GetActorLocation().Z;
	PreviousFrameYaw = GetActorRotation().Yaw;
}

// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	MovementStateCheck();
	Acceleration();

	if (GetCharacterMovement()->IsMovingOnGround() || StateMachine->GetCurrentEnumState() != ECharacterState::DefaultState)
	{
		if (StateMachine->GetCurrentEnumState() == ECharacterState::DefaultState)
		{
			LastInteractedWall = nullptr;
			CanJump = true;
		}
		InternalCoyoteTimer = 0;
	}
	//This for coyote for normal jump from the ground, as every other ability have their internal timer
	//But if i were to quit them, their function would break, eg I need to be in wall run to do coyote for wall run.
	//So I cannot do a global coyote here unfortunately.
	else if (CanJump && StateMachine->GetCurrentEnumState() == ECharacterState::DefaultState && InternalCoyoteTimer <= CoyoteTime)
	{
		InternalCoyoteTimer += DeltaTime;
	}
	else if (InternalCoyoteTimer > CoyoteTime && !CanDash && CanJump)
	{
		CanJump = false;
		CanDash = true;
	}
	

	if (StateMachine != nullptr) StateMachine->UpdateStateMachine();
	if (DebugVelocity) DebugSpeed();
}

// Called to bind functionality to input
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AMyCharacter::JumpAndDash);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Move
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AMyCharacter::NoMovementInput);
		//EnhancedInputComponent->BindAction(PreciseMoveAction, ETriggerEvent::Triggered, this, &AMyCharacter::PreciseMovement);
		//EnhancedInputComponent->BindAction(PreciseMoveAction, ETriggerEvent::Completed, this, &AMyCharacter::DisablePreciseMovement);

		//Look
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyCharacter::Look);

		//Sliding
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Started, this, &AMyCharacter::Slide);
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Completed, this, &AMyCharacter::ResetSlide);

		//Looking Back
		//EnhancedInputComponent->BindAction(LookBackAction, ETriggerEvent::Triggered, this, &AMyCharacter::LookBack);
		//EnhancedInputComponent->BindAction(LookBackAction, ETriggerEvent::Completed, this, &AMyCharacter::LookFront);
	}
}

#pragma endregion

#pragma region Movement

void AMyCharacter::JumpAndDash()
{
	if (StateMachine == nullptr) return;

	FVector JumpVector = GetActorUpVector() * JumpStrength;
	StateMachine->OverrideJump(JumpVector);

	//Internal coyote timer will be 0 for every other ability, as they have their own internal timer. They reset jump manually.
	//SetStateBool will check automatically if we can exit the current state.
	if (InternalCoyoteTimer <= CoyoteTime && CanJump && SetStateBool(ECharacterState::DefaultState))
	{
		GetCharacterMovement()->Velocity = GetCharacterMovement()->Velocity.GetSafeNormal() * MaxRunningSpeed;
		GetCharacterMovement()->Velocity.Z = 0;
		GetCharacterMovement()->UpdateComponentVelocity();
		GetCharacterMovement()->AddImpulse(JumpVector, true);
		CanJump = false;
		CanDash = true;
		HandleJumpEvent();
	}
	else if (CanDash && !CanJump && SetStateBool(ECharacterState::AirDashing))
	{
		CanDash = false;
	}
}


void AMyCharacter::Acceleration()
{
	/*
	if (CurrentMovementState == ECharacterMovementState::Walking)
	{
		GetCharacterMovement()->MaxWalkSpeed = PreciseWalkingSpeed;
	}
	*/
	/*else*/
	if (CurrentMovementState == ECharacterMovementState::Fell)
	{
		GetCharacterMovement()->MaxWalkSpeed = RunningStateSpeedMinimum * PostFallAccelerationTime->GetFloatValue(
			AccelerationTimer * (1 - CalculatedPostFallMultiplier));
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = RunningStateSpeedMinimum + (MaxRunningSpeed - RunningStateSpeedMinimum) * RunningAccelerationTime->
			GetFloatValue(AccelerationTimer);
	}

	if (StateMachine != nullptr)
	{
		StateMachine->OverrideAcceleration(GetCharacterMovement()->MaxWalkSpeed);
	}

	CameraJitter(GetCharacterMovement()->MaxWalkSpeed);
}

void AMyCharacter::MovementStateCheck()
{
	//TODO if a player doesnt move for a while, should it still stay in FELL or override it to walking? is that even possible? 

	if (CurrentMovementState != ECharacterMovementState::Fell && Falling && GetCharacterMovement()->IsMovingOnGround() && FallDistance > 0)
	{
		if (DebugFall && GEngine)
		{
			const FString Text = FString::Printf(TEXT("Fall distance: %f. This is %f units."), FallDistance, FallDistance / FallZDistanceUnit);
			GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Red, Text);
		}
		const float PotentialPenalty = FallDistance / FallZDistanceUnit * PenaltyMultiplierPerFallUnit;
		CalculatedPostFallMultiplier = PotentialPenalty >= MaxPenaltyMultiplier ? MaxPenaltyMultiplier : PotentialPenalty;
		AccelerationTimer = 0;
		CurrentMovementState = ECharacterMovementState::Fell;
		Falling = false;
		GetCharacterMovement()->Velocity = FVector::ZeroVector;
		PlayerFellEvent();
	}


	if (CurrentMovementState != ECharacterMovementState::Fell && StateMachine->GetCurrentEnumState() != ECharacterState::AirDashing &&
		GetHorizontalVelocity() <= 1)
	{
		CurrentMovementState = ECharacterMovementState::Idle;
	}


	if (CurrentMovementState == ECharacterMovementState::Idle || (CurrentMovementState == ECharacterMovementState::Fell && GetHorizontalVelocity() >=
		RunningStateSpeedMinimum))
	{
		CurrentMovementState = ECharacterMovementState::Running;
		AccelerationTimer = 0;
		EnteredRunningEvent();
	}


	//Falling calculation has to happen after acceleration mode setting otherwise, it will never detect falling as Z height is set every frame.
	if (Falling && DebugFall && GEngine && FallDistance >= FallZDistanceUnit)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, "Will trigger fall on landing");
	}
	if (StateMachine != nullptr)
	{
		if (GetCharacterMovement()->Velocity.Z != 0 && StateMachine->GetCurrentState()->DoesItCountTowardsFalling())
		{
			Falling = true;
			if (!DisableExtraGravity)
			{
				GetCharacterMovement()->AddForce(-GetActorUpVector() * 980 * GetCharacterMovement()->Mass * (GravityWhileFalling - 1));
			}
			FallDistance = FMath::Abs(FallStartZ - GetActorLocation().Z) - FallZDistanceUnit;
		}
		else if (GetCharacterMovement()->Velocity.Z == 0 && StateMachine->GetCurrentState()->DoesItCountTowardsFalling()
			|| !StateMachine->GetCurrentState()->DoesItCountTowardsFalling())
		{
			FallStartZ = GetActorLocation().Z;
		}
		else
		{
			FallStartZ = FallStartZ >= GetActorLocation().Z ? FallStartZ : GetActorLocation().Z;
		}
	}
}


void AMyCharacter::Move(const FInputActionValue& Value)
{
	AccelerationTimer += GetWorld()->GetDeltaSeconds();
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (MovementVector != PreviousMovementVector)
	{
		//AccelerationTimer = 0;
	}

	//Should this be below?
	//PreviousMovementVector = MovementVector;

	if (StateMachine != nullptr)
	{
		//If the current state has implemented override, it will override the movement vector. Otherwise it will return back the original vector.
		StateMachine->OverrideMovementInput(MovementVector);
	}

	//TODO because players keep input and velocity doesnt change, when they suddenly change from left to right the acceleration does not reset.

	if (Controller != nullptr)
	{
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}

	PreviousMovementVector = MovementVector;

	StateMachine->DetectStates();
}

void AMyCharacter::NoMovementInput()
{
	if (StateMachine != nullptr)
	{
		StateMachine->OverrideNoMovementInputEvent();
	}

	PreviousMovementVector = FVector2d::ZeroVector;
	AccelerationTimer = 0;
}

/*
void AMyCharacter::PreciseMovement()
{
	if (GetMovementComponent()->IsMovingOnGround() && GetHorizontalVelocity() >= PreciseWalkingSpeed)
	{
		CurrentMovementState = ECharacterMovementState::Walking;
		EnteredWalkingEvent();
	}
}

void AMyCharacter::DisablePreciseMovement()
{
	if (GetMovementComponent()->IsMovingOnGround() && CurrentMovementState == ECharacterMovementState::Walking)
	{
		CurrentMovementState = ECharacterMovementState::Idle;
		//AccelerationTimer = 0;
	}
}
*/


void AMyCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>() * (2.0f * Sensitivity);

	if (StateMachine != nullptr)
	{
		//If the current state has implemented override, it will override the movement vector. Otherwise it will return back the original vector.
		StateMachine->OverrideCameraInput(LookAxisVector);
	}

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

#pragma endregion

void AMyCharacter::LookBack()
{
	FrontCam->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	BackCam->SetActive(true);
	FrontCam->SetActive(false);
	LookBackTimeSlowEvent();
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), LookBackTimeScale->GetFloatValue(LookBackTimer));
	if (LookBackTimer < LookBackTimeScale->FloatCurve.GetLastKey().Time)
	{
		LookBackTimer += GetWorld()->GetDeltaSeconds();
	}
}

//This (LookFront) executes once! That's why I have put an async function.
void AMyCharacter::LookFront()
{
	FrontCam->Activate(true);
	BackCam->Activate(false);
	LookBackTimer = LookBackTimeScale->FloatCurve.GetLastKey().Time;
	auto TurnTimeToNormalAsync = Async(EAsyncExecution::Thread, [&]() { TurnTimeBackAsync(); });
}

void AMyCharacter::TurnTimeBackAsync()
{
	while (LookBackTimer >= 0)
	{
		const float TimeDilation = LookBackTimeScale->GetFloatValue(LookBackTimer);
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), TimeDilation);
		LookBackTimer -= GetWorld()->GetDeltaSeconds();
		LookBackTimeSlowEvent();
		FPlatformProcess::Sleep(GetWorld()->GetDeltaSeconds());
	}
	LookBackTimer = 0;
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1);
}


void AMyCharacter::Slide()
{
	SetState(ECharacterState::Sliding);
}


void AMyCharacter::ResetSlide()
{
	if (StateMachine != nullptr && StateMachine->GetCurrentEnumState() == ECharacterState::Sliding) StateMachine->ManualExitState();
}

#pragma region Debug
void AMyCharacter::DebugSpeed() const
{
	if (GEngine)
	{
		const FString vertical = FString::Printf(TEXT("Falling speed: %lf"), GetCharacterMovement()->Velocity.Z);
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Yellow, *vertical);
		const FString horizontal = FString::Printf(TEXT("Movement speed: %lf"), GetHorizontalVelocity());
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Yellow, *horizontal);
	}
}

bool AMyCharacter::SetStateBool(const ECharacterState NewState) const
{
	if (StateMachine != nullptr)
	{
		return StateMachine->SetState(NewState);
	}
	return false;
}

void AMyCharacter::SetState(const ECharacterState NewState) const
{
	if (StateMachine != nullptr)
	{
		StateMachine->SetState(NewState);
	}
}

void AMyCharacter::CameraJitter(float& WalkSpeed)
{
	if (bUseControllerRotationYaw && GetCharacterMovement()->IsMovingOnGround() && CurrentMovementState != ECharacterMovementState::Fell)
	{
		const float AbsDifference = FMath::Abs(GetActorRotation().Yaw - PreviousFrameYaw);
		const float RoundedDifference = FMath::FloorToFloat(AbsDifference * 10.0f) / 10.0f;;
		const float Step = 100.0f / (180 - JitterSlowMinAngle);
		if (DebugCameraLeftRight)
		{
			const FString AngleDiff = FString::Printf(TEXT("Angle Diff Between Frames: %f"), RoundedDifference);
			GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Blue, AngleDiff);
		}

		if (AbsDifference > JitterSlowMinAngle)
		{
			WalkSpeed *= (1 - RoundedDifference * Step * JitterSlowPercentageStrength / 100);
			//CurrentMovementState = ECharacterMovementState::Walking;
		}
	}
	PreviousFrameYaw = GetActorRotation().Yaw;
}


#pragma endregion
