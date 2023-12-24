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
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AMyCharacter::JumpAndDash);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyCharacter::Move);

		//Looking
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
		//Jump();
		GetCharacterMovement()->AddImpulse(FVector(0, 0, JumpStrength), true);
		StateMachine->SetState(ECharacterState::DefaultState);
	}
	else if (StateMachine != nullptr && StateMachine->GetCurrentEnumState() != ECharacterState::AirDashing && TouchedGroundOrWall)
	{
		StateMachine->SetState(ECharacterState::AirDashing);
		TouchedGroundOrWall = false;
	}
}

// Movement

void AMyCharacter::Acceleration(const float& DeltaTime)
{
	accelerationTimer += DeltaTime; //This resets the moment the player stops giving movement inputs.

	if (CurrentMovementState == EMovementState::Idle || CurrentMovementState == EMovementState::Walking)
	{
		GetCharacterMovement()->MaxWalkSpeed = RunningStateSpeedMinimum * WalkingAccelerationTime->GetFloatValue(accelerationTimer);
	}
	else if (CurrentMovementState == EMovementState::Fell)
	{
		GetCharacterMovement()->MaxWalkSpeed = RunningStateSpeedMinimum * PostFallAccelerationTime->GetFloatValue(
			accelerationTimer * (1 - CalculatedPostFallMultiplier));
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = RunningStateSpeedMinimum + (MaxRunningSpeed - RunningStateSpeedMinimum) * RunningAccelerationTime->
			GetFloatValue(accelerationTimer);
	}

	if (StateMachine != nullptr && !StateMachine->IsCurrentStateNull())
	{
		StateMachine->OverrideAcceleration(GetCharacterMovement()->MaxWalkSpeed);
	}
}

void AMyCharacter::MovementStateCheck()
{
	//Falling stuff
	if (CurrentMovementState != EMovementState::Fell && IsFalling() && StateMachine != nullptr && StateMachine->GetCurrentState()->CountTowardsFalling)
	{
		FallingTimer += GetWorld()->GetDeltaSeconds();
		Falling = true;
	}

	//TODO if a player doesnt move for a while, should it still stay in FELL or override it to walking?
	if (CurrentMovementState != EMovementState::Fell)
	{
		if (Falling && GetCharacterMovement()->IsMovingOnGround() && FallingTimer >= MinimumFallTime)
		{
			const float Diff = (FallingTimer - MinimumFallTime) * PenaltyMultiplierPerSecond;
			CalculatedPostFallMultiplier = Diff >= MaxPenaltyMultiplier ? MaxPenaltyMultiplier : Diff;
			accelerationTimer = 0;
			CurrentMovementState = EMovementState::Fell;
			ResetFalling();
			GetCharacterMovement()->Velocity = FVector::ZeroVector;
		}
		else if (CurrentMovementState != EMovementState::Idle && GetHorizontalVelocity() <= 1)
		{
			CurrentMovementState = EMovementState::Idle;
			accelerationTimer = 0;
		}
		else if (CurrentMovementState != EMovementState::Walking && GetHorizontalVelocity() > 1 && GetHorizontalVelocity() < RunningStateSpeedMinimum)
		{
			CurrentMovementState = EMovementState::Walking;
			accelerationTimer = 0;
		}
	}

	if (CurrentMovementState != EMovementState::Running && GetHorizontalVelocity() >= RunningStateSpeedMinimum)
	{
		CurrentMovementState = EMovementState::Running;
		accelerationTimer = 0;
	}
}


void AMyCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (StateMachine != nullptr && !StateMachine->IsCurrentStateNull())
	{
		//If the current state has implemented override, it will override the movement vector
		//Otherwise it will return back the original vector.
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
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();


	if (StateMachine != nullptr && !StateMachine->IsCurrentStateNull())
	{
		//If the current state has implemented override, it will override the movement vector
		//Otherwise it will return back the original vector.
		StateMachine->OverrideCameraInput(LookAxisVector);
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
	if (StateMachine != nullptr) StateMachine->SetState(ECharacterState::Sliding);
}

bool AMyCharacter::IsFalling() const
{
	return GetCharacterMovement()->Velocity.Z <= 0 && GetCharacterMovement()->IsFalling();
}

void AMyCharacter::ResetSlide()
{
	if (StateMachine != nullptr && StateMachine->GetCurrentEnumState() == ECharacterState::Sliding) StateMachine->ManualExitState();
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
			/*
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
			*/
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
