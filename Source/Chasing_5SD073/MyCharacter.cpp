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
	StateMachine->SetState(ECharacterState::DefaultState);
	GetCharacterMovement()->MaxCustomMovementSpeed = MaxRunningSpeed;
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
	if (bDebugSpeed) DebugSpeed();
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
		GetCharacterMovement()->AddImpulse(FVector(0, 0, JumpStrength), true);
		StateMachine->SetState(ECharacterState::DefaultState);
	}
	else if (StateMachine != nullptr && StateMachine->GetCurrentEnumState() != ECharacterState::AirDashing && TouchedGroundOrWall)
	{
		StateMachine->SetState(ECharacterState::AirDashing);
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
}

void AMyCharacter::MovementStateCheck()
{
	//Falling
	if (CurrentMovementState != EMovementState::Fell && GetCharacterMovement()->IsFalling() && StateMachine != nullptr && StateMachine->GetCurrentState()->DoesItCountTowardsFalling())
	{
		FallingTimer += GetWorld()->GetDeltaSeconds();
		GEngine->AddOnScreenDebugMessage(-1,0,FColor::Red,"Falling");
		Falling = true;
	}

	//TODO if a player doesnt move for a while, should it still stay in FELL or override it to walking?
	if (CurrentMovementState != EMovementState::Fell)
	{
		if (Falling && GetCharacterMovement()->IsMovingOnGround() && FallingTimer >= MinimumFallTime)
		{
			const float Diff = (FallingTimer - MinimumFallTime) * PenaltyMultiplierPerSecond;
			CalculatedPostFallMultiplier = Diff >= MaxPenaltyMultiplier ? MaxPenaltyMultiplier : Diff;
			AccelerationTimer = 0;
			CurrentMovementState = EMovementState::Fell;
			ResetFalling();
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

	// since mouse input is using LookAxisVector, im checking how much the player is moving the mouse on x
	// but its hard to know how much the mouse is input-ing
	if (LookAxisVector.X >= CameraJitter || LookAxisVector.X <= -CameraJitter)
	{
		//GetCharacterMovement()->Velocity *= slowPrecentage; //Need a better solution
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
	if (StateMachine != nullptr) StateMachine->SetState(ECharacterState::Sliding);
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
		const FString msg = FString::Printf(TEXT("Player speed: %lf"), GetCharacterMovement()->Velocity.Length());
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, *msg);
	}
}
#pragma endregion