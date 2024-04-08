// Fill out your copyright notice in the Description page of Project Sliding.

#pragma once

#include "CoreMinimal.h"
#include "StateComponentBase.h"
#include "Components/ActorComponent.h"
#include "SlidingStateComponent.generated.h"

struct FSurfaceInfo
{
	float Angle;
	FVector Normal;
	bool MovingDown;
	bool IsSlopedSurface;
};


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CHASING_5SD073_API USlidingStateComponent : public UStateComponentBase
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USlidingStateComponent();

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
	virtual void OverrideAcceleration(UCharacterStateMachine& SM, float& NewSpeed) override;
	virtual void OverrideDebug() override;
	bool DetectGround() const;

private:
	void ResetCapsuleSize();
	void ShrinkCapsuleSize();
	bool IsUnderObject() const;
	bool SweepCapsuleSingle(FVector& Start, FVector& End) const;
	bool IsOnSlope(FSurfaceInfo& SlopeInfo) const;
	FSurfaceInfo GetSurfaceInfo() const;

	bool IsTimerOn(const FTimerHandle& Timer) const;
	void StopTimer(FTimerHandle& Timer) const;

private:
	UPROPERTY(EditAnywhere, Category= "Settings",
		meta = (Tooltip = "This does not represent the max time of the slide. The numbers represent the max speed of the player."))
	UCurveFloat* SlideSpeedCurve;

	UPROPERTY(EditAnywhere, Category= "Settings")
	UCurveFloat* CapsuleSizeCurve;

	UPROPERTY(EditAnywhere, Category= "Settings", meta = (Tooltip = "Max Slide Duration (seconds)", ClampMin = 0))
	float MaxSlideDuration = 0;

	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ToolTip = "It is a % value, limiting the intensity of left right input.", ClampMin = 0))
	float SlidingLeftRightMovementModifier = 0;

	UPROPERTY(EditAnywhere, Category= "Settings",  meta = (ToolTip = "If true, looking up and down is as fast as usual."))
	bool OnlyModifyCameraLeftRight = false;

	UPROPERTY(EditAnywhere, Category= "Settings",
		meta = (ToolTip = "It is a % value, limiting the intensity of camera input. All directions by default", ClampMin = 0))
	float SlidingCameraModifier = 0;

	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ClampMin = 0))
	float AboutToFallDetectionDistance = 0;

	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ToolTip = "The time it takes for the capsule grow back into it's original size", ClampMin = 0))
	float CapsuleResizeDuration = 1.0f;

	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ToolTip = "The time it takes for the capsule shrink", ClampMin = 0))
	float CapsuleShrinkDuration = 1.0f;

	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ToolTip = "The minimum height of the capsule during a slide", ClampMin = 0))
	float CapsuleMinHeight = 55.0f;
	
	///Internal
	float InternalTimer = 0;
	float CameraFullHeight = 0;
	float CameraReducedHeight = 0;

	bool IsCapsuleShrunk = false;
	FTimerHandle CapsuleSizeResetTimer;
	FTimerHandle CapsuleSizeShrinkTimer;

	float ExpandTime = 0.0f;
	float ShrinkTime = 0.0f;
	
	float CapsuleMaxHeight = 96.0f;
	float CapsuleCurrentHeight = 0;

	float CameraMinHeight = 15.0f;
	float CameraMaxHeight = 60.0f;
	float CameraCurrentHeight = 0;
};
