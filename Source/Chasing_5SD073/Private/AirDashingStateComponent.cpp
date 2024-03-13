// Fill out your copyright notice in the Description page of Project Settings.


#include "AirDashingStateComponent.h"

#include "GameFramework/CharacterMovementComponent.h"

// Sets default values for this component's properties
UAirDashingStateComponent::UAirDashingStateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	// ...
}


// Called when the game starts
void UAirDashingStateComponent::BeginPlay()
{
	Super::BeginPlay();
	// ...
}


// Called every frame
void UAirDashingStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// ...
}

bool UAirDashingStateComponent::OnSetStateConditionCheck(UCharacterStateMachine& SM)
{
	return PlayerCharacter->GetHorizontalVelocity() > 0 && !PlayerCharacter->GetCharacterStateMachine()->IsThisCurrentState(*this);
}

void UAirDashingStateComponent::OnEnterState(UCharacterStateMachine& SM)
{
	Super::OnEnterState(SM);
	InternalTimer = 0;
	InitialForwardVector = PlayerCharacter->GetFirstPersonCameraComponent()->GetComponentRotation().Vector();
	const float Speed = PlayerCharacter->GetHorizontalVelocity();
	PlayerMovement->Velocity = FVector(InitialForwardVector.X * Speed, InitialForwardVector.Y * Speed, PlayerMovement->Velocity.Z);
	
	HorizontalVelocity = PlayerCharacter->GetHorizontalVelocity();
	AccelerationTimer = PlayerCharacter->AccelerationTimer;
}

void UAirDashingStateComponent::OnUpdateState(UCharacterStateMachine& SM)
{
	Super::OnUpdateState(SM);

	SweepOnDash = true;
	
	//Instead of relying on physics, (cause it is janky) I am manually calculating where the player is supposed to be.
	InternalTimer += GetWorld()->GetDeltaSeconds();
	FVector NextFrameLocation = PlayerCharacter->GetActorLocation() + InitialForwardVector * AirDashDistance * (InternalTimer / AirDashTime) *
		(PlayerCharacter->GetHorizontalVelocity() / PlayerCharacter->GetMaxRunningSpeed()); //Last part provides boost on current speed

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(PlayerCharacter);
	
	// Perform a sweep test to check for potential collisions
	if (FHitResult HitResult; GetWorld()->SweepSingleByChannel(
		HitResult,
		PlayerCharacter->GetActorLocation(),
		NextFrameLocation,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeCapsule(PlayerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius(),
		PlayerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), Params))
	{
		// Get the box's edges
		FBox Bounds = HitResult.GetActor()->GetComponentsBoundingBox();

		// Define the edges
		TArray Lines = GetEdges(Bounds);

		if (EnableEdgeCorrectionDebug)
		{
			for (auto Line : Lines)
			{
				DrawDebugLine(GetWorld(), Line.Key, Line.Value, FColor::Green, false, 5, 0, 5);
			}
		}

		// Check if the impact point is close to any of the edges
		for (const Line& Line : Lines)
		{
			float MinDis = 0.0f;
			FVector p;

			const auto a = Line.Key;
			const auto b = Line.Value;
			const auto c = HitResult.ImpactPoint;

			const auto Ab = b - a;
			const auto Ac = c - a;

			const auto t = FVector::DotProduct(Ac, Ab) / FVector::DotProduct(Ab, Ab);

			if (t <= 0) {
				MinDis = FVector::Distance(a, c);
			}
			else if (t >= 1) {
				MinDis = FVector::Distance(b, c);
			}
			else {
				p = a + Ab * t;
				MinDis = FVector::Distance(p, c);
			}
			
			if (MinDis < EdgeDistThreshold)
			{
				if (EnableEdgeCorrectionDebug)
				{
					UE_LOG(LogTemp, Warning, TEXT("Min Dis: %f"), MinDis);
					DrawDebugPoint(GetWorld(), p, 15, FColor::Red, false, 5, 0);
				}
				
				NextFrameLocation.Z += EdgeCorrectionAmount;

				SweepOnDash = false;
				
				break;
			}
		}
	}

	if (EnableEdgeCorrectionDebug)
		UE_LOG(LogTemp, Warning, TEXT("Sweeping: %hs"), SweepOnDash ? "true" : "false");
	
	PlayerCharacter->SetActorLocation(NextFrameLocation, SweepOnDash); //true prevent player 'dashing' inside of a wall, stop at hitting.

	if (InternalTimer >= AirDashTime) SM.ManualExitState();
}

void UAirDashingStateComponent::OnExitState(UCharacterStateMachine& SM)
{
	Super::OnExitState(SM);

	PlayerCharacter->AccelerationTimer = AccelerationTimer;
	
	//GetWorld()->GetTimerManager().ClearTimer(SlideTimerHandle);
	//GetWorld()->GetTimerManager().SetTimer(SlideTimerHandle, this, &UAirDashingStateComponent::AddSlide, GetWorld()->GetDeltaSeconds(), true);
}

void UAirDashingStateComponent::OverrideMovementInput(UCharacterStateMachine& SM, FVector2d& NewMovementVector)
{
	Super::OverrideMovementInput(SM, NewMovementVector);
	//This prevents any player inputs doing air dash.

	if (NewMovementVector.Y > 0) IsHoldingW = true;
	else IsHoldingW = false;
	
	NewMovementVector = FVector2d::ZeroVector;
}

void UAirDashingStateComponent::OverrideDebug()
{
	if (!DebugMechanic) return;
	
	Super::OverrideDebug();
	const FVector Start = GetOwner()->GetActorLocation();
	DrawDebugLine(GetWorld(), Start, Start + GetOwner()->GetActorForwardVector() * AirDashDistance * (PlayerCharacter->GetHorizontalVelocity() /
		              PlayerCharacter->GetMaxRunningSpeed()), DebugColor, false, 0, 0, 3);
}

TArray<Line> UAirDashingStateComponent::GetEdges(const FBox& Bounds) const
{
	const FVector Min = Bounds.Min;
	const FVector Max = Bounds.Max;
		
	TArray Corners = {
		FVector(Min.X, Min.Y, Max.Z),
		FVector(Min.X, Max.Y, Max.Z),
		FVector(Max.X, Min.Y, Max.Z),
		FVector(Max.X, Max.Y, Max.Z)
	};

	TArray Lines = {
		TPair<FVector, FVector>(Corners[0], Corners[1]),
		TPair<FVector, FVector>(Corners[1], Corners[3]),
		TPair<FVector, FVector>(Corners[3], Corners[2]),
		TPair<FVector, FVector>(Corners[2], Corners[0])
	};

	return Lines;
}

void UAirDashingStateComponent::AddSlide()
{
	if (PlayerMovement->IsMovingOnGround())
	{
		PlayerCharacter->AddMovementInput(PlayerCapsule->GetForwardVector(), 3.0f, false);
		
		if (const bool HoldingW = IsKeyDown(EKeys::W); !HoldingW)
			GetWorld()->GetTimerManager().ClearTimer(SlideTimerHandle);
	}
}
