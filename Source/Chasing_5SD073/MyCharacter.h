// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CharacterStateMachine.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MyCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UCharacterStateMachine;

UENUM(BlueprintType)
enum class EMovementState : uint8
{
	Idle,
	Walking,
	Running,
	Fell
};

UCLASS()
class CHASING_5SD073_API AMyCharacter : public ACharacter
{
	GENERATED_BODY()
	
public:
	// Sets default values for this character's properties
	AMyCharacter();

private:
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

	UPROPERTY(VisibleAnywhere, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FrontCam;

	UPROPERTY(VisibleAnywhere, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* BackCam;

	UPROPERTY()
	UCharacterStateMachine* StateMachine = nullptr;
	
	UPROPERTY(EditAnywhere, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* AirDashAction;

	UPROPERTY(EditAnywhere, Category= "Input")
	UInputAction* SlideAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookBackAction;

	UPROPERTY(EditAnywhere, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void Acceleration(const float& DeltaTime);
	void MovementStateCheck();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Slide();
	void LookBack();
	void LookFront();
	void JumpAndDash();
	void DebugSpeed() const;
	//Returns true if successful
	bool SetStateBool(ECharacterState NewState) const;
	void SetState(ECharacterState NewState) const;
	void CameraJitter(float& WalkSpeed);

public:
	void ResetDash() { TouchedGroundOrWall = true; }
	void ResetFalling()
	{
		Falling = false;
	}
	void ResetSlide();
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE EMovementState GetCharacterMovementState() const { return CurrentMovementState; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE UCameraComponent* GetFirstPersonCameraComponent() const { return FrontCam; }

	FORCEINLINE float GetHorizontalVelocity() const
	{
		return FVector2d(GetCharacterMovement()->Velocity.X, GetCharacterMovement()->Velocity.Y).Size();
	}
	FORCEINLINE void SetWhetherTouchedGroundOrWall(const bool b) { TouchedGroundOrWall = b; }
	FORCEINLINE bool GetWhetherTouchedGroundOrWall() const { return TouchedGroundOrWall; }
	FORCEINLINE float GetMaxRunningSpeed() const { return MaxRunningSpeed; }
	FORCEINLINE UCharacterStateMachine* GetCharacterStateMachine() const { return StateMachine; }
	FORCEINLINE USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Movement Settings", DisplayName= "Current Movement State")
	EMovementState CurrentMovementState = EMovementState::Idle;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Walking")
	UCurveFloat* WalkingAccelerationTime;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Falling")
	UCurveFloat* PostFallAccelerationTime;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Falling")
	bool DisableExtraGravity = false;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Falling", meta = (ClampMin = 0))
	float FallZDistanceUnit;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Falling", meta = (ClampMin = 0, ClampMax = 1))
	float PenaltyMultiplierPerFallUnit;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Falling", meta = (ClampMin = 0, ClampMax = 1))
	float MaxPenaltyMultiplier;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Running")
	UCurveFloat* RunningAccelerationTime;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Running", meta = (ClampMin = 0))
	float RunningStateSpeedMinimum = 800;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Running", meta = (ClampMin = 0))
	float MaxRunningSpeed;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Jump", meta = (ClampMin = 0))
	float JumpStrength;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Camera")
	float JitterSlowMinAngle;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Camera", meta = (ClampMin = 0, Tooltip = "At 1 Strength, it is 1% per 1 angle difference"))
	float JitterSlowPercentageStrength;
	
	UPROPERTY(EditAnywhere, Category = "Movement Settings|Debug")
	bool DebugVelocity;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Debug")
	bool DebugFall;
	
	UPROPERTY(EditAnywhere, Category = "Movement Settings|Debug")
	bool DebugCameraLeftRight;
	
	float AccelerationTimer;
	float CalculatedPostFallMultiplier;
	UPROPERTY(VisibleAnywhere, Category = "Movement Settings|Debug")
	float FallStartZ;
	UPROPERTY(VisibleAnywhere, Category = "Movement Settings|Debug")
	float FallDistance;
	
	bool TouchedGroundOrWall;
	bool Falling;
	

	float PrevYaw = 0;
};