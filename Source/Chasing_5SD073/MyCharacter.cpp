// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"


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
	//currentSpeed = normalSpeed;
}

// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//Ray();

	//if (moving)
	//{
	//	Acceleration(DeltaTime);
	//}
//
	//SliderRaycast();
	
	if (GetCharacterMovement()->IsFalling() && !landed)
	{
		//landed = false;
		//GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("Player is falling")));
		fallingTimer += DeltaTime;
		
	}
	else
	{
		//fallingTimer = 0;
	}
	
	GroundRaycast(DeltaTime);
	//DebugSpeed();
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
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Triggered, this, &AMyCharacter::Slide);
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::None, this, &AMyCharacter::ResetSize);

		//Looking Back
		EnhancedInputComponent->BindAction(LookBackAction, ETriggerEvent::Triggered, this,
		                                   &AMyCharacter::LookBack);
		EnhancedInputComponent->BindAction(LookBackAction, ETriggerEvent::Completed, this,
		                                   &AMyCharacter::LookFront);
	}
}

void AMyCharacter::Acceleration()
{
	// when u are not falling the acceleration time doesnt get effected
	float currentAcceleration = maximumPlayerSpeed / accelerationSpeedRate;

	//landed after falling
	if (landed)
	{

	if (fallingTimer >= maxFallingPenaltyTime) fallingTimer = maxFallingPenaltyTime;
		
		currentAcceleration = maximumPlayerSpeed / accelerationSpeedRate * (1 - (fallingTimer / maxFallingPenaltyTime) * maxFallingSpeedSlowPenalty);

		if(GetCharacterMovement()->Velocity.Length() >= maximumPlayerSpeed)
		{
			landed = false;
			fallingTimer = 0;
		}
	}

	// accelerationSpeedRate = how long (seconds) it should take to reach maximum speed

	GetCharacterMovement()->MaxAcceleration = currentAcceleration;
	
}

void AMyCharacter::Move(const FInputActionValue& Value)
{
	//moving = true;

	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
	Acceleration();
}

void AMyCharacter::ResetSize()
{
	FVector3d scale = GetActorScale3D();
	FVector3d origin = FVector3d(1, 1, 1);
	if (scale != origin)
	{
		SetActorScale3D(scale * 2);
	}
	//DebugSize();
}

void AMyCharacter::LookBack()
{
	FString fru = moving ? "true" : "false";
	UE_LOG(LogTemp, Display, TEXT("%s"), *fru);
	FrontCam->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	BackCam->SetActive(true);
	FrontCam->SetActive(false);
	//FrontCam->Activate(false);
}

void AMyCharacter::LookFront()
{
	FrontCam->Activate(true);
	BackCam->Activate(false);
}

void AMyCharacter::Ray()
{
	FVector start = GetActorLocation();
	FVector forward = FrontCam->GetForwardVector();
	FVector down = -FrontCam->GetUpVector();
	start = FVector(start.X + (forward.X * 100), start.Y + (forward.Y * 100), start.Z + (forward.Z * 100));
	FVector end = start + (forward * 50);
	FHitResult hit;

	if (GetWorld())
	{
		bool actorHit = GetWorld()->LineTraceSingleByChannel(hit, start, end, ECC_Pawn, FCollisionQueryParams(),
		                                                     FCollisionResponseParams());
		DrawDebugLine(GetWorld(), start, end, FColor::Red, false, 0.5f, 0.0f, 5.0f);
		if (actorHit && hit.GetActor())
		{
			// is ground check
			float dis = GetDistanceTo(hit.GetActor());
			//if (GEngine)
			//{
			//	const FString msg = FString::Printf(TEXT("diss: %f"), dis);
			//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, *msg);
			//}

			// slope check
			if (hit.GetActor()->ActorHasTag(TEXT("Slope")))
			{
				if (GEngine)
				{
					const FString msg = FString::Printf(TEXT("diss: %s"), *hit.GetActor()->GetName());
					GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, *msg);
				}
			}
			else
			{
			}
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red,
			                                 FString::Printf(TEXT("You are htting: %s"), *hit.GetActor()->GetName()));
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red,
			                                 FString::Printf(TEXT("Impact point: %s"), *hit.ImpactPoint.ToString()));

			FVector hitActorPoint = hit.ImpactPoint;
			FVector playerPoint = GetActorLocation();


			//FName slider = "Slider";
			//FName temp = FName(hit.GetActor()->Tags[0]);
			//
			////float dis = GetDistanceTo(hit.GetActor()->Tags);
			////UE_LOG(LogTemp,Warning,TEXT("Hit actor: %s"),*hit.GetActor()->Tags;
			//if (temp != nullptr)
			//{
			//	if (temp == slider)
			//	{
			//		if (GEngine)
			//		{
			//			const FString msg = FString::Printf(TEXT("diss: %s"), *temp.ToString());
			//			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, *msg);
			//		}
			//	}
			//}
		}
	}
}

void AMyCharacter::GroundRaycast(float DeltaTime)
{
	FVector start = GetActorLocation();
	FVector bounds = GetCapsuleComponent()->Bounds.BoxExtent;
	float minZ = bounds.Z + 0.1f;
	start -= FVector(0, 0, minZ);
	FVector down = -GetActorUpVector();
	//start = FVector(start.X + (down.X * 100), start.Y + (down.Y * 100), start.Z + (down.Z * 100));
	FVector end = start + (down * 10000.f);
	FHitResult hit;

	if (GetWorld())
	{
		bool actorHit = GetWorld()->LineTraceSingleByChannel(hit, start, end, ECC_Pawn, FCollisionQueryParams(),
		                                                     FCollisionResponseParams());
		//DrawDebugLine(GetWorld(), start, end, FColor::Red, false, 0.5f, 0.0f, 5.0f);
		if (actorHit)
		{
			// is ground check
			FVector hitActorPoint = hit.ImpactPoint;
			FVector playerPoint = start;
			FVector dis;
			float distance = dis.Distance(playerPoint,hitActorPoint);

			//UE_LOG(LogTemp, Log, TEXT("Distance between player and actor: %f"), distance);

			if(distance <= 70 && GetCharacterMovement()->IsFalling())
			{
				landed = true;
			}
		}
	}
}

void AMyCharacter::SliderRaycast()
{
	FVector start = GetActorLocation();
	FVector forward = FrontCam->GetForwardVector();
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
				GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("U are on a Slope")));
			}
			else
			{
				onSlope = false;
			}
		}
	}
}

void AMyCharacter::SpeedReset()
{
	moving = false;
	//currentSpeed = normalSpeed;
	//GetCharacterMovement()->MaxWalkSpeed = normalSpeed;
	accelerationTimer=0;
}

void AMyCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}

	if (LookAxisVector.X >= cameraJitter || LookAxisVector.X <= -cameraJitter)
	{
		GetCharacterMovement()->Velocity *= slow_precentage;
	}
}

void AMyCharacter::Slide()
{
	FVector3d scale = GetActorScale3D();
	FVector3d origin = FVector3d(1, 1, 1);
	if (scale == origin)
	{
		SetActorScale3D(scale / 2);
		DebugSize();
	}
}

void AMyCharacter::DebugSpeed()
{
	if (GEngine)
	{
		const FString msg = FString::Printf(TEXT("Player speed: %lf"), GetCharacterMovement()->Velocity.Length());
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, *msg);
	}
}

void AMyCharacter::DebugSize()
{
	FVector scale = GetActorScale3D();
	UE_LOG(LogTemp, Log, TEXT("Actor location: %s"), *scale.ToString());
}
