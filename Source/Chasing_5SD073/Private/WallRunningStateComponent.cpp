// Fill out your copyright notice in the Description page of Project Settings.


#include "WallRunningStateComponent.h"

#include "GameFramework/CharacterMovementComponent.h"


// Sets default values for this component's properties
UWallRunningStateComponent::UWallRunningStateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UWallRunningStateComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}

void UWallRunningStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


void UWallRunningStateComponent::OnEnterState(UCharacterStateMachine& SM)
{
	if (PlayerCharacter->GetWallMechanicHitResult()->GetActor() == PreviousWall && PreviousWall != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1,2, FColor::Red,"Same wall");
		SM.ManualExitState();
		return;
	}

	Super::OnEnterState(SM);

	
	PreviousWall = PlayerCharacter->GetWallMechanicHitResult()->GetActor();

	PlayerCharacter->bUseControllerRotationYaw = false;


	InternalTimer = 0;
	InternalGravityScale = 0;
	PlayerMovement->Velocity.Z = 0;

	//TODO Need to adjust this. Not super precise.

	const FVector CharacterToWall = (PlayerCharacter->GetWallMechanicHitResult()->ImpactPoint - PlayerCharacter->GetActorLocation()).GetSafeNormal();

	const float DotProduct = FVector::DotProduct(PlayerCharacter->GetActorRightVector(), CharacterToWall);

	if (DotProduct >= 0)
	{
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Red, "It's on the right side.");
		WallOrientation = Right;
	}
	else
	{
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Red, "It's on the left side.");
		WallOrientation = Left;
	}

	//Rotating player to face the direction they would be going
	RotatePlayerAlongsideWall(*PlayerCharacter->GetWallMechanicHitResult());
}

void UWallRunningStateComponent::OnUpdateState(UCharacterStateMachine& SM)
{
	Super::OnUpdateState(SM);
	

	const FVector CounterGravityForce = FVector(0,0,PlayerMovement->Mass * 980);
	const FVector GravityForce = FVector(0,0,-PlayerMovement->Mass * 980) * WallRunGravityCurve->GetFloatValue(InternalTimer);

	PlayerMovement->AddForce(CounterGravityForce + GravityForce);


	if (!CheckWhetherStillWallRunning()) SM.ManualExitState();
}

void UWallRunningStateComponent::OnExitState(UCharacterStateMachine& SM)
{
	Super::OnExitState(SM);
	PlayerCharacter->bUseControllerRotationYaw = true;
}

void UWallRunningStateComponent::OverrideMovementInput(FVector2d& NewMovementVector)
{
	RotatePlayerAlongsideWall(HitResult);
	NewMovementVector.X = 0;
}

void UWallRunningStateComponent::OverrideCameraInput(UCameraComponent& Camera, FVector2d& NewRotationVector)
{
	Super::OverrideCameraInput(Camera, NewRotationVector);

	// FRotator NewRotation = Camera.GetComponentRotation();
	// NewRotation.Yaw += 90;
	// Camera.SetWorldRotation(NewRotation);
}

void UWallRunningStateComponent::RotatePlayerAlongsideWall(const FHitResult& Hit)
{
	FVector RotatedVector;

	if (WallOrientation == Left)
	{
		RotatedVector.X = Hit.Normal.Y;
		RotatedVector.Y = -Hit.Normal.X;
	}

	if (WallOrientation == Right)
	{
		RotatedVector.X = -Hit.Normal.Y;
		RotatedVector.Y = -Hit.Normal.X;
	}

	PlayerCharacter->SetActorRotation(RotatedVector.Rotation());
}

bool UWallRunningStateComponent::CheckWhetherStillWallRunning()
{
	const FVector Start = PlayerCharacter->GetActorLocation();
	const int SideMultiplier = WallOrientation == Right ? 1 : -1; //return - 1 if it is left, to negate RightVector()
	//Idk what to do about 0.75f, make it public?
	const FVector End = Start + PlayerCharacter->GetActorRightVector() * PlayerCharacter->GetWallCheckDistance() * SideMultiplier * 2;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(GetOwner());

	const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, CollisionParams);
	if (bDebug) DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, -1);

	//If moving on ground, cancel wall run outright, otherwise, check the side if we are still wall running
	if (PlayerMovement->IsMovingOnGround())
	{
		PreviousWall = nullptr;
		return false;
	}
	return bHit;
}
