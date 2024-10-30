// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateComponentBase.h"
#include "Components/ActorComponent.h"
#include "WallRunningStateComponent.generated.h"

UENUM(BlueprintType)
enum EWallOrientation { None, Left, Right };


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CHASING_5SD073_API UWallRunningStateComponent : public UStateComponentBase
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UWallRunningStateComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual bool OnSetStateConditionCheck(UCharacterStateMachine& SM) override;
	virtual void OnEnterState(UCharacterStateMachine& SM) override;
	virtual void OnUpdateState(UCharacterStateMachine& SM) override;
	virtual void OnExitState(UCharacterStateMachine& SM) override;

	virtual void OverrideMovementInput(UCharacterStateMachine& SM, FVector2d& NewMovementVector) override;
	virtual void OverrideCameraInput(UCharacterStateMachine& SM, FVector2d& NewRotationVector) override;
	virtual void OverrideDebug() override;
	virtual void OverrideDetectState(UCharacterStateMachine& SM) override;
	virtual void OverrideJump(UCharacterStateMachine& SM, FVector& JumpVector) override;
	virtual void OverrideNoMovementInputEvent(UCharacterStateMachine& SM) override;


	UFUNCTION(BlueprintPure)
	FORCEINLINE EWallOrientation GetWallOrientation() const { return WallOrientation; }

private:
	FVector RotatePlayerAlongsideWall(const FHitResult& Hit) const;
	bool CheckWhetherStillWallRunning();
	void BlockContinuousWall();
	bool CloseToGround() const;

	UPROPERTY(EditAnywhere, Category= "Settings",
		meta = (Tooltip = "This stops sensors from being an insta trigger, preventing accidentals.", ClampMin = 0))
	float WallRunTriggerDelay = 0;

	UPROPERTY(EditAnywhere, Category= "Settings")
	UCurveFloat* WallRunGravityCurve;

	UPROPERTY(EditAnywhere, Category = "Settings", meta = (ClampMin = 0))
	float WallCheckDistance = 0;

	UPROPERTY(EditAnywhere, Category = "Settings", meta = (ClampMin = 0))
	float WallRunSideAngle = 0;

	UPROPERTY(EditAnywhere, Category = "Settings", meta = (ClampMin = 0))
	float MinimumDistanceFromGround = 0;

	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ClampMin = 0))
	float MaxWallRunDuration = 0;

	UPROPERTY(EditAnywhere, Category= "Settings")
	bool DisableTapJump;	
	
	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ClampMin = 0))
	float WallRunTapAngle = 80.0f;

	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ClampMin = 0, Tooltip = "This takes the original JumpStrength as a base and multiplies it by this value."))
	float TapJumpForceUpModifier = 0.35f;
	
	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ClampMin = 0, Tooltip = "This takes the original JumpStrength as a base and multiplies it by this value."))
	float TapJumpForceSideModifier = 1.85f;
	
	

	//Internal
	EWallOrientation WallOrientation = None;
	FHitResult HitResult;
	FVector LastPointOnWall = FVector::ZeroVector;
	FVector PlayerForwardVectorOnEnter;
	FVector PlayerUpVectorOnEnter;
	//This booleans lets us utilise coyote time after we have stopped wall running, while keeping gravity and direction as if we were wall running.
	bool NoLongerWallRunning = false;
	float TriggerTimer;
	float WallRunTimer;
	bool EnteringWallRun = false;
	bool TouchedGround = false;
	UPROPERTY() TArray<AActor*> NonInteractableWalls;
};
