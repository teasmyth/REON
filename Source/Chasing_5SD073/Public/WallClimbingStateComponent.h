// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateComponentBase.h"
#include "Components/ActorComponent.h"
#include "WallClimbingStateComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CHASING_5SD073_API UWallClimbingStateComponent : public UStateComponentBase
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UWallClimbingStateComponent();

	virtual bool OnSetStateConditionCheck(UCharacterStateMachine& SM) override;
	virtual void OnEnterState(UCharacterStateMachine& SM) override;
	virtual void OnUpdateState(UCharacterStateMachine& SM) override;
	virtual void OnExitState(UCharacterStateMachine& SM) override;

	virtual void OverrideMovementInput(UCharacterStateMachine& SM, FVector2d& NewMovementVector) override;
	virtual void OverrideNoMovementInputEvent(UCharacterStateMachine& SM) override;
	virtual void OverrideDebug() override;
	virtual void OverrideDetectState(UCharacterStateMachine& SM) override;
	virtual void OverrideJump(UCharacterStateMachine& SM, FVector& JumpVector) override;

private:
	void DetectWallClimb();
	bool CheckLedge() const;
	bool CheckLeg() const;
	bool CheckBody() const;
	bool CloseToGround() const;


	UPROPERTY(EditAnywhere, Category= "Settings")
	UCurveFloat* WallClimbIntensityCurve = nullptr;
	
	UPROPERTY(EditAnywhere, Category= "Settings",
		meta = (Tooltip = "This stops Wall Climb sensors from being an insta trigger, preventing accidentals."))
	float WallClimbTriggerDelay = 0;

	UPROPERTY(EditAnywhere, Category = "Settings", meta = (ClampMin = 0))
	float WallCheckDistance = 0;

	UPROPERTY(EditAnywhere, Category = "Settings", meta = (ClampMin = 0))
	float WallClimbAngle = 0;

	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ClampMin = 0))
	float WallClimbSpeed = 0;

	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ClampMin = 0))
	float LedgeGrabCheckZOffset = 0;

	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ClampMin = 0))
	float LedgeGrabCheckDistance = 0;

	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ClampMin = 0))
	float MaxWallClimbDuration = 0;
	
	UPROPERTY(EditAnywhere, Category= "Settings")
	bool DisableTapWallFacingJump = false;
	
	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ClampMin = 0, Tooltip = "This takes the original JumpStrength as a base and multiplies it by this value."))
	float WallFacingJumpUpForceMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ClampMin = 0, Tooltip = "This takes the original JumpStrength as a base and multiplies it by this value."))
	float WallFacingJumpBackForceMultiplier = 1.0f;

	//Internal
	FHitResult HitResult;
	//FHitResult PrevResult;
	//const FHitResult EmptyResult;
	float InternalTimer = 0;
	float TriggerTimer = 0;
	bool DisableInput = false; //for coyote time.
	bool JustJumped = false;
};
