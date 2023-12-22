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
	WallMechanicsCheck();
	if (GetCharacterMovement()->IsMovingOnGround())
	{
		TouchedGroundOrWall = true;
	}
	
	if (GetCharacterMovement()->IsFalling() && !landed)
	{
		fallingTimer += DeltaTime;
	}

	StateMachine->UpdateStateMachine();

	// on slope automatically slide, onSlope is checked in SliderRaycast()
	//if (onSlope) Slide();

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
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Completed, this, &AMyCharacter::ResetCharacterState);

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

	if (StateMachine != nullptr && StateMachine->CurrentState != nullptr)
	{
		StateMachine->OverrideAcceleration(GetCharacterMovement()->MaxWalkSpeed);
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
	//This way the Z movement (for example jumping) does not alter the speed calculation.
	const float VerticalSpeed = FVector2d(GetMovementComponent()->Velocity.X, GetMovementComponent()->Velocity.Y).Size();
	
	if (CurrentMovementState != EMovementState::Idle && VerticalSpeed <= 1)
	{
		CurrentMovementState = EMovementState::Idle;
		accelerationTimer = 0;
	}
	else if (CurrentMovementState != EMovementState::Walking && VerticalSpeed > 1 && VerticalSpeed < RunningStateSpeedMinimum)
	{
		CurrentMovementState = EMovementState::Walking;
		accelerationTimer = 0;
	}
	else if (CurrentMovementState != EMovementState::Running && VerticalSpeed >= RunningStateSpeedMinimum)
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
	if (StateMachine == nullptr) return;

	if (CurrentMovementState == EMovementState::Running && StateMachine->CurrentEnumState != ECharacterState::Sliding)
	{
		StateMachine->SetState(ECharacterState::Sliding);
	}
}

void AMyCharacter::WallMechanicsCheck()
{
	//Need to see the debug regardless of checking the wall or not.

	//Helper lambda to rotate vector.
	auto RotateVector = [](const FVector& InVector, const float AngleInDegrees, const float Length) {
		const FRotator Rotation = FRotator(0.0f, AngleInDegrees, 0.0f);
		const FQuat QuatRotation = FQuat(Rotation);
		FVector RotatedVector = QuatRotation.RotateVector(InVector);
		RotatedVector.Normalize();
		RotatedVector *= Length;
		return RotatedVector;
	};
	
	const FVector Start = GetActorLocation();
	const FVector End = Start + GetActorRotation().Vector() * WallCheckForwardDistance;
	
	if (bDebugWallMechanics)
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, -1, 0, 5);
		
		DrawDebugLine(GetWorld(), Start, Start + RotateVector(GetActorRotation().Vector(), WallRunAngle, WallCheckForwardDistance), FColor::Blue, false, -1, 0, 5);
		DrawDebugLine(GetWorld(), Start, Start + RotateVector(GetActorRotation().Vector(), -WallRunAngle, WallCheckForwardDistance), FColor::Blue, false, -1, 0, 5);

		//Side check debug. Right and Left
		DrawDebugLine(GetWorld(), Start, Start + RotateVector(GetActorRotation().Vector(), WallRunSideAngle, WallCheckSideDistance), FColor::Red, false, -1, 0, 5);
		DrawDebugLine(GetWorld(), Start, Start + RotateVector(GetActorRotation().Vector(), -WallRunSideAngle, WallCheckSideDistance), FColor::Red, false, -1, 0, 5);
	}
	
	//calculate whether wer are trying to wall run or wall climb, if at all
	if (CurrentMovementState != EMovementState::Running || StateMachine == nullptr || StateMachine != nullptr &&
		(StateMachine->GetCurrentEnumState() == ECharacterState::WallClimbing || StateMachine->GetCurrentEnumState() == ECharacterState::WallRunning))
	{
		return;
	}
	
	FHitResult HitResult;
	
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);

	const bool bAimHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, CollisionParams);
	

	if (bAimHit) //Prioritizing player's aim.
	{
		//Setting up reference for wall mechanics.
		WallMechanicHitResult = &HitResult;
		TouchedGroundOrWall = true;
		
		// Calculate the angle of incidence
		const float AngleInDegrees = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(HitResult.Normal, -GetActorForwardVector())));

		// AngleInDegrees now contains the angle between the wall and the actor's forward vector
		if (AngleInDegrees >= WallRunAngle && !GetCharacterMovement()->IsMovingOnGround())
		{
			StateMachine->SetState(ECharacterState::WallRunning);
			if (bDebugWallMechanics)GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, "Can Wall Run");
		}
		else
		{
			StateMachine->SetState(ECharacterState::WallClimbing);
			if (bDebugWallMechanics) GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Blue, "Can Wall Climb (Not Implemented)");
		}
		
		return;
	}

	if (GetCharacterMovement()->IsMovingOnGround()) return; //We don' need side detection on the ground.s

	//If player's aim fails but standing right next to ONE wall then it is also wall run.
	FHitResult RightSide;
	const bool bRightSideHit = GetWorld()->LineTraceSingleByChannel(RightSide, Start, Start + RotateVector(GetActorRotation().Vector(), WallRunSideAngle, WallCheckSideDistance),
	                                                                ECC_Visibility, CollisionParams);

	const bool bLeftSideHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, Start + RotateVector(GetActorRotation().Vector(), -WallRunSideAngle, WallCheckSideDistance),
	                                                               ECC_Visibility, CollisionParams);


	if (bRightSideHit && !bLeftSideHit || !bRightSideHit && bLeftSideHit)
	{
		if (bDebugWallMechanics) GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, "Can Wall Run");

		if (bRightSideHit)
		{
			WallMechanicHitResult = &RightSide;
			if (bDebugWallMechanics) DrawDebugBox(GetWorld(), RightSide.ImpactPoint, FVector(10, 10, 10), FQuat::Identity, FColor::Green, false, 100);
		}
		else
		{
			WallMechanicHitResult = &HitResult;
			if (bDebugWallMechanics) DrawDebugBox(GetWorld(), HitResult.ImpactPoint, FVector(10, 10, 10), FQuat::Identity, FColor::Green, false, 100);
		}

		TouchedGroundOrWall = true;
		StateMachine->SetState(ECharacterState::WallRunning);
		return;
	}


	WallMechanicHitResult = nullptr;
}

void AMyCharacter::ResetCharacterState()
{
	if (StateMachine != nullptr) StateMachine->ManualExitState();
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
