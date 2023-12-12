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
	//Mesh1P->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	dash = false;
	dashOnce = false;
}

// Called when the game starts or when spawned
void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	StateMachine = GetComponentByClass<UCharacterStateMachine>();
	GetCharacterMovement()->MaxCustomMovementSpeed = MaxRunningSpeed;
}

// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	MovementStateCheck();
	Acceleration(DeltaTime);

	if (GetCharacterMovement()->IsFalling() && !landed)
	{
		fallingTimer += DeltaTime;
	}
	else
	{
		//
	}

	// air dash starts
	if (GetCharacterMovement()->IsFalling())
	{
		dash = true; // while u in the air, u can dash
	}
	else
	{
		dash = false; // reset back 
		dashOnce = true; //
		airDashDelayTimer = 0;
		startDelay = false;
		startDash = false;
	}

	if (startDelay)
	{
		airDashDelayTimer += DeltaTime;

		if (airDashDelayTimer > airDashDelay)
		{
			startDash = true;
			DashAction();
		}
	}
	// air dash ends

	// on slope automatically slide, onSlope is checked in SliderRaycast()
	if (onSlope) Slide();

	// debugs
	if (debugGroundRaycast) GroundRaycast();
	if (debugSlideRaycast) SliderRaycast();
	if (debugSpeed) DebugSpeed();
	if (debugLanding) DebugLanding();
}

// Called to bind functionality to input
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::None, this, &AMyCharacter::SpeedReset);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyCharacter::Look);

		//Sliding
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Started, this, &AMyCharacter::Slide);
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Completed, this, &AMyCharacter::ResetCharacterState);

		//Looking Back
		EnhancedInputComponent->BindAction(LookBackAction, ETriggerEvent::Triggered, this, &AMyCharacter::LookBack);
		EnhancedInputComponent->BindAction(LookBackAction, ETriggerEvent::Completed, this, &AMyCharacter::LookFront);

		//AirDash
		EnhancedInputComponent->BindAction(AirDashAction, ETriggerEvent::Triggered, this, &AMyCharacter::AirDash);
	}
}

#pragma endregion

#pragma region Movement

void AMyCharacter::DashAction()
{
	// the actual dash movement, using a ray to detect new location to dash to

	if (startDash)
	{
		FVector start = GetActorLocation();
		FVector forward = FrontCam->GetForwardVector();
		start = FVector(start.X + (forward.X * airDashDistance), start.Y + (forward.Y * airDashDistance),
		                start.Z + (forward.Z * airDashDistance));
		FVector end = start + forward;

		if (GetWorld())
		{
			DrawDebugLine(GetWorld(), start, end, FColor::Red, false, 0.5f, 0.0f, 5.0f);

			GetCharacterMovement()->GravityScale = gravityOrigin;
			SetActorLocation(end, true);

			dashOnce = false;
			startDash = false;
			startDelay = false;
			airDashDelayTimer = 0;
		}
	}
}

void AMyCharacter::SetCharacterSpeed(const float& NewSpeed) const
{
	GetCharacterMovement()->MaxWalkSpeed = FMath::Clamp(NewSpeed, 0, MaxPossibleSpeed);
}

void AMyCharacter::AddCharacterSpeed(const float& NewSpeed) const
{
	GetCharacterMovement()->MaxWalkSpeed += FMath::Clamp(NewSpeed, 0, MaxPossibleSpeed);
}

// Movement

void AMyCharacter::Acceleration(const float& DeltaTime)
{
	accelerationTimer += DeltaTime; //This is reset the moment the player stops giving movement inputs.

	//float currentAcceleration = GetCharacterMovement()->MaxCustomMovementSpeed / accelerationSpeedRate;


	if (CurrentMovementState == EMovementState::Idle || CurrentMovementState == EMovementState::Walking)
	{
		GetCharacterMovement()->MaxWalkSpeed = RunningStateSpeedMinimum * WalkingAccelerationTime->GetFloatValue(accelerationTimer);
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = RunningStateSpeedMinimum + (MaxRunningSpeed - RunningStateSpeedMinimum) * RunningAccelerationTime->
			GetFloatValue(accelerationTimer);
	}

	/*
	//landed after falling
	if (landed)
	{
		if (fallingTimer >= maxFallingPenaltyTime)
		{
			fallingTimer = maxFallingPenaltyTime;
		}

		currentAcceleration = GetCharacterMovement()->MaxCustomMovementSpeed / accelerationSpeedRate * (1 - (
			fallingTimer / maxFallingPenaltyTime) * maxFallingSpeedSlowPenalty);

		if (GetCharacterMovement()->Velocity.Length() >= GetCharacterMovement()->MaxCustomMovementSpeed)
		{
			landed = false;
			fallingTimer = 0;
		}
	}
	*/

	//GetCharacterMovement()->MaxAcceleration = currentAcceleration;
	//getCurrentAccelerationRate = currentAcceleration;
}

void AMyCharacter::MovementStateCheck()
{
	if (CurrentMovementState != EMovementState::Idle && GetMovementComponent()->Velocity.Size() <= 1)
	{
		CurrentMovementState = EMovementState::Idle;
		accelerationTimer = 0;
	}
	else if (CurrentMovementState != EMovementState::Walking && GetMovementComponent()->Velocity.Size() > 1 && GetMovementComponent()->Velocity.Size()
		< RunningStateSpeedMinimum)
	{
		CurrentMovementState = EMovementState::Walking;
		accelerationTimer = 0;
	}
	else if (CurrentMovementState != EMovementState::Running && GetMovementComponent()->Velocity.Size() >= RunningStateSpeedMinimum)
	{
		CurrentMovementState = EMovementState::Running;
		accelerationTimer = 0;
	}
}


void AMyCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (StateMachine != nullptr && StateMachine->CurrentState != nullptr)
	{
		//If the current state has implemented override, it will override the movement vector
		//Otherwise it will return back the original vector.
		StateMachine->CurrentState->OverrideMovement(MovementVector);
	}

	//Rounding the number up to 1 if its at 0.95 (or above) in cases when you look up and down a little which alters the input due to pitch/yaw.
	if (MovementVector.Y >= 0.95f) MovementVector.Y = 1;

	if (Controller != nullptr)
	{
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AMyCharacter::SpeedReset()
{
	//accelerationTimer = 0; Moved to movement state check
}

void AMyCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();


	if (StateMachine != nullptr && StateMachine->CurrentState != nullptr)
	{
		//If the current state has implemented override, it will override the movement vector
		//Otherwise it will return back the original vector.
		StateMachine->CurrentState->OverrideCamera(*FrontCam, LookAxisVector);
	}


	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}

	// since mouse input is using LookAxisVector, im checking how much the player is moving the mouse on x
	// but its hard to know how much the mouse is input-ing
	if (LookAxisVector.X >= cameraJitter || LookAxisVector.X <= -cameraJitter)
	{
		//GetCharacterMovement()->Velocity *= slowPrecentage; //Need a better solution
	}
}

void AMyCharacter::LookBack()
{
	FrontCam->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	BackCam->SetActive(true);
	FrontCam->SetActive(false);
}

void AMyCharacter::LookFront()
{
	// calls this function after LookBack()
	FrontCam->Activate(true);
	BackCam->Activate(false);
}

void AMyCharacter::Slide()
{
	if (StateMachine == nullptr) return;
	
	if (CurrentMovementState == EMovementState::Running && StateMachine->CurrentEnumState != ECharacterState::Sliding)
	{
		StateMachine->SetState(ECharacterState::Sliding);
		FVector2d tmpvector = FVector2d::Zero();
		StateMachine->CurrentState->OverrideCamera(*FrontCam, tmpvector);
	}
}

void AMyCharacter::WallMechanicsCheck()
{
	//calculate whether wer are trying to wall run or wall climb, if at all
	//if (wallclimb) -> statemachine.setstate(wallclimb)
	//if (wallrun) -> statemachine.setstate(wallrun)
}

void AMyCharacter::ResetCharacterState()
{
	if (StateMachine != nullptr) StateMachine->ResetState();
}

void AMyCharacter::AirDash(const FInputActionValue& Value)
{
	if (Controller != nullptr)
	{
		if (!dash || !dashOnce)
		{
			return;
		}

		startDelay = true;
		dashValue = Value.Get<FVector>();
		// after u pressed space when u are in the air(has to be jumping) ur gravity becomes low
		GetCharacterMovement()->GravityScale = gravityLow;
	}
}

#pragma endregion

#pragma region RayCast
// Raycast

void AMyCharacter::GroundRaycast()
{
	FVector start = GetActorLocation();
	FVector bounds = GetCapsuleComponent()->Bounds.BoxExtent;
	float minZ = bounds.Z + 0.1f;
	start -= FVector(0, 0, minZ);
	FVector down = -GetActorUpVector();
	FVector end = start + (down * 10000.f);
	FHitResult hit;

	if (GetWorld())
	{
		bool actorHit = GetWorld()->LineTraceSingleByChannel(hit, start, end, ECC_Pawn, FCollisionQueryParams(),
		                                                     FCollisionResponseParams());
		DrawDebugLine(GetWorld(), start, end, FColor::Red, false, 0.5f, 0.0f, 5.0f);
		if (actorHit)
		{
			// is ground check
			FVector hitActorPoint = hit.ImpactPoint;
			FVector playerPoint = start;
			FVector dis;
			float distance = dis.Distance(playerPoint, hitActorPoint);

			if (distance <= 3 && GetCharacterMovement()->IsFalling())
			{
				landed = true;
			}

			if (debugGroundRaycast)
			{
				UE_LOG(LogTemp, Log, TEXT("Distance between player and actor: %f"), distance);
			}
		}
	}
}

void AMyCharacter::SliderRaycast()
{
	FVector start = GetActorLocation();
	FVector down = -FrontCam->GetUpVector();
	start = FVector(start.X + (down.X * 100), start.Y + (down.Y * 100), start.Z + (down.Z * 100));
	FVector end = start + (down * 50);
	FHitResult hit;

	if (GetWorld())
	{
		bool actorHit = GetWorld()->LineTraceSingleByChannel(hit, start, end, ECC_Pawn, FCollisionQueryParams(),
		                                                     FCollisionResponseParams());
		DrawDebugLine(GetWorld(), start, end, FColor::Red, false, 0.5f, 0.0f, 5.0f);
		if (actorHit && hit.GetActor())
		{
			// slope check
			if (hit.GetActor()->ActorHasTag(TEXT("Slope")))
			{
				onSlope = true;
			}
			else
			{
				onSlope = false;
			}

			if (debugGroundRaycast)
			{
				UE_LOG(LogTemp, Warning, TEXT("slope %s"), ( onSlope ? TEXT("true") : TEXT("false") ));
			}
		}
	}
}

#pragma endregion

#pragma region Debug
// Debug

void AMyCharacter::DebugSpeed()
{
	if (GEngine)
	{
		const FString msg = FString::Printf(TEXT("Player speed: %lf"), GetCharacterMovement()->Velocity.Length());
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, *msg);
	}
}

void AMyCharacter::DebugLanding()
{
	UE_LOG(LogTemp, Warning, TEXT("falling %s"),
	       ( GetCharacterMovement()->IsFalling() ? TEXT("true") : TEXT("false") ));
}

void AMyCharacter::DebugSize()
{
	FVector scale = GetActorScale3D();
	UE_LOG(LogTemp, Log, TEXT("Actor location: %s"), *scale.ToString());
}

#pragma endregion
