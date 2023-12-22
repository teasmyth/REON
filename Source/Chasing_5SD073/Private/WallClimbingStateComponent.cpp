// Fill out your copyright notice in the Description page of Project Settings.


#include "WallClimbingStateComponent.h"

// Sets default values for this component's properties
UWallClimbingStateComponent::UWallClimbingStateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UWallClimbingStateComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}


// Called every frame
void UWallClimbingStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UWallClimbingStateComponent::OnEnterState(UCharacterStateMachine& SM)
{
	Super::OnEnterState(SM);
	PlayerCharacter->bUseControllerRotationYaw = false;
	InternalTimer = 0;
	PlayerMovement->Velocity = FVector(0, 0, PlayerMovement->Velocity.Z / 4);
	PlayerMovement->UpdateComponentVelocity();
	const FVector Normal = -PlayerCharacter->GetWallMechanicHitResult()->Normal;
	PlayerCharacter->SetActorRotation(Normal.Rotation());
}

void UWallClimbingStateComponent::OnUpdateState(UCharacterStateMachine& SM)
{
	Super::OnUpdateState(SM);
	
	PlayerMovement->AddForce(FVector(0, 0, PlayerMovement->Mass * 980));
	InternalTimer += GetWorld()->GetDeltaSeconds();
	
	if (InternalTimer >= MaxWallClimbDuration || CheckLedge() && CheckLeg())
	{
		SM.ManualExitState();
	}
}

void UWallClimbingStateComponent::OnExitState(UCharacterStateMachine& SM)
{
	Super::OnExitState(SM);
	PlayerCharacter->bUseControllerRotationYaw = true;
}

void UWallClimbingStateComponent::OverrideMovementInput(UCharacterStateMachine& SM, FVector2d& NewMovementVector)
{
	Super::OverrideMovementInput(SM, NewMovementVector);


	if (NewMovementVector.Y <= 0)
	{
		SM.ManualExitState();
	}

	const float CurrentSpeed = WallClimbSpeed * GetWorld()->GetDeltaSeconds();
	const float VerticalMovement = CurrentSpeed * (InternalTimer / MaxWallClimbDuration);
	PlayerCharacter->SetActorLocation(PlayerCharacter->GetActorLocation() + FVector(0.0f, 0.0f, VerticalMovement), true);
	NewMovementVector = FVector2d::ZeroVector;
}

bool UWallClimbingStateComponent::CheckLedge() const
{
	const FVector Start = PlayerCapsule->GetComponentLocation() + FVector(0, 0, LedgeGrabCheckZOffset);
	const FVector End = Start + PlayerCharacter->GetActorForwardVector() * LedgeGrabCheckDistance;

	if (Debug)
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, -1, 0, 5);
	}
	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(GetOwner());

	//Checking above the head.

	if (!GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, CollisionParams))
	{
		return true;
	}
	return false;
}

bool UWallClimbingStateComponent::CheckLeg() const
{
	const FVector Start = PlayerCapsule->GetComponentLocation() - FVector(0, 0, PlayerCapsule->GetScaledCapsuleHalfHeight());
	const FVector End = Start + PlayerCharacter->GetActorForwardVector() * LedgeGrabCheckDistance;

	if (Debug)
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, -1, 0, 5);
	}

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(GetOwner());

	//Checking above the head.
	if (!GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, CollisionParams))
	{
		return true;
	}
	return false;
}
