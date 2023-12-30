// Fill out your copyright notice in the Description page of Project Settings.

#include "MyCharacter.h"
#include "CharacterStateMachine.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "StateComponentBase.h"
#include "GameFramework/CharacterMovementComponent.h"

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
	PrevYaw = GetActorRotation().Yaw;
}

// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	MovementStateCheck();
	Acceleration(DeltaTime);
	if (GetCharacterMovement()->IsMovingOnGround())
	{
		TouchedGroundOrWall = true;
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

		//Move & Look
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyCharacter::Look);

		//Sliding
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Started, this, &AMyCharacter::Slide);
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Completed, this, &AMyCharacter::ResetSlide);

		//Looking Back
		EnhancedInputComponent->BindAction(LookBackAction, ETriggerEvent::Triggered, this, &AMyCharacter::LookBack);
		EnhancedInputComponent->BindAction(LookBackAction, ETriggerEvent::Completed, this, &AMyCharacter::LookFront);
	}
}

#pragma endregion

#pragma region Movement

void AMyCharacter::JumpAndDash()
{
	if (GetCharacterMovement()->IsMovingOnGround() || StateMachine != nullptr &&
		(StateMachine->GetCurrentEnumState() == ECharacterState::WallRunning || StateMachine->GetCurrentEnumState() == ECharacterState::WallClimbing))
	{
		GetCharacterMovement()->AddImpulse(GetActorUpVector() * JumpStrength, true);
		SetState(ECharacterState::DefaultState);
	}
	else if (TouchedGroundOrWall && SetStateBool(ECharacterState::AirDashing))
	{
		TouchedGroundOrWall = false;
	}
}


void AMyCharacter::Acceleration(const float& DeltaTime)
{
	AccelerationTimer += DeltaTime; //This resets every time movement state is changed

	if (CurrentMovementState == EMovementState::Idle || CurrentMovementState == EMovementState::Walking)
	{
		GetCharacterMovement()->MaxWalkSpeed = RunningStateSpeedMinimum * WalkingAccelerationTime->GetFloatValue(AccelerationTimer);
	}
	else if (CurrentMovementState == EMovementState::Fell)
	{
		GetCharacterMovement()->MaxWalkSpeed = RunningStateSpeedMinimum * PostFallAccelerationTime->GetFloatValue(
			AccelerationTimer * (1 - CalculatedPostFallMultiplier));
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = RunningStateSpeedMinimum + (MaxRunningSpeed - RunningStateSpeedMinimum) * RunningAccelerationTime->
			GetFloatValue(AccelerationTimer);
	}

	if (StateMachine != nullptr && !StateMachine->IsCurrentStateNull())
	{
		StateMachine->OverrideAcceleration(GetCharacterMovement()->MaxWalkSpeed);
	}

	CameraJitter(GetCharacterMovement()->MaxWalkSpeed);
}

void AMyCharacter::MovementStateCheck()
{
	//TODO if a player doesnt move for a while, should it still stay in FELL or override it to walking? is that even possible? i dont think I cant get max key
	if (CurrentMovementState != EMovementState::Fell)
	{
		if (Falling && GetCharacterMovement()->IsMovingOnGround() && FallDistance > 0)
		{
			if (DebugFall && GEngine)
			{
				const FString Text = FString::Printf(TEXT("Fall distance: %f. This is %f units."), FallDistance, FallDistance / FallZDistanceUnit);
				GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Red, Text);
			}
			const float PotentialPenalty = FallDistance / FallZDistanceUnit * PenaltyMultiplierPerFallUnit;
			CalculatedPostFallMultiplier = PotentialPenalty >= MaxPenaltyMultiplier ? MaxPenaltyMultiplier : PotentialPenalty;
			AccelerationTimer = 0;
			CurrentMovementState = EMovementState::Fell;
			Falling = false;
			GetCharacterMovement()->Velocity = FVector::ZeroVector;
		}
		else if (CurrentMovementState != EMovementState::Idle && GetHorizontalVelocity() <= 1)
		{
			CurrentMovementState = EMovementState::Idle;
			AccelerationTimer = 0;
		}
		else if (CurrentMovementState != EMovementState::Walking && GetHorizontalVelocity() > 1 && GetHorizontalVelocity() < RunningStateSpeedMinimum)
		{
			CurrentMovementState = EMovementState::Walking;
			AccelerationTimer = 0;
		}
	}

	if (CurrentMovementState != EMovementState::Running && GetHorizontalVelocity() >= RunningStateSpeedMinimum)
	{
		CurrentMovementState = EMovementState::Running;
		AccelerationTimer = 0;
	}

	//Falling calculation has to happen after acceleration mode setting otherwise, it will never detect falling as Z height is set every frame.
	if (Falling && DebugFall && GEngine && FallDistance >= FallZDistanceUnit)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, "Will trigger fall on landing");
	}
	if (StateMachine != nullptr)
	{
		if (GetCharacterMovement()->Velocity.Z < 0 && StateMachine->GetCurrentState()->DoesItCountTowardsFalling())
		{
			Falling = true;
			if (!DisableExtraGravity) GetCharacterMovement()->AddForce(-GetActorUpVector() * 980 * GetCharacterMovement()->Mass);
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
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (StateMachine != nullptr && !StateMachine->IsCurrentStateNull())
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

	StateMachine->DetectStates();
}

void AMyCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (StateMachine != nullptr && !StateMachine->IsCurrentStateNull())
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
}

void AMyCharacter::LookFront()
{
	FrontCam->Activate(true);
	BackCam->Activate(false);
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
	else return false;
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
	if (bUseControllerRotationYaw && GetCharacterMovement()->IsMovingOnGround() && CurrentMovementState != EMovementState::Fell)
	{
		const float AbsDifference = FMath::Abs(GetActorRotation().Yaw - PrevYaw);
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
			CurrentMovementState = EMovementState::Walking;
		}
	}
	PrevYaw = GetActorRotation().Yaw;
}
#pragma endregion
