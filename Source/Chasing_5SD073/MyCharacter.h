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
enum class ECharacterMovementState : uint8
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

	//UPROPERTY(EditAnywhere, Category=Input, meta=(AllowPrivateAccess = "true"))
	//UInputAction* PreciseMoveAction;

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

	void Acceleration();
	void MovementStateCheck();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Slide();
	void LookBack();
	void LookFront();
	void JumpAndDash();
	void DebugSpeed() const;
	bool SetStateBool(ECharacterState NewState) const; //Returns true if successful
	void SetState(ECharacterState NewState) const;
	void CameraJitter(float& WalkSpeed);
	void TurnTimeBackAsync();
	void NoMovementInput();
	//void PreciseMovement();
	//void DisablePreciseMovement();

	//Jump event extension for blueprint.
	UFUNCTION(BlueprintImplementableEvent, Category = "Character Custom Events")
	void HandleJumpEvent(); //Ignore the warning by Rider. It is implemented.

	//Look back event extension for blueprint.
	UFUNCTION(BlueprintImplementableEvent, Category = "Character Custom Events")
	void LookBackTimeSlowEvent(); //Same

	UFUNCTION(BlueprintImplementableEvent, Category = "Character Custom Events")
	void EnteredWalkingEvent();

	UFUNCTION(BlueprintImplementableEvent, Category = "Character Custom Events")
	void EnteredRunningEvent();
	
	//Fell event extension for blueprint.
	UFUNCTION(BlueprintImplementableEvent, Category = "Character Custom Events")
	void PlayerFellEvent();

public:
	void ResetDash() { CanDash = true; }
	void ResetJump() { CanJump = true; }

	void ResetFalling()
	{
		Falling = false;
	}

	void ResetSlide();

	UFUNCTION(BlueprintPure)
	FORCEINLINE ECharacterMovementState GetCharacterMovementState() const { return CurrentMovementState; }

	UFUNCTION(BlueprintPure)
	FORCEINLINE FVector GetCharacterMovementInput() const { return FVector(PreviousMovementVector.X, PreviousMovementVector.Y, 0);}

	UFUNCTION(BlueprintPure)
	FORCEINLINE UCameraComponent* GetFirstPersonCameraComponent() const { return FrontCam; }

	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetHorizontalVelocity() const
	{
		return FVector2d(GetCharacterMovement()->Velocity.X, GetCharacterMovement()->Velocity.Y).Size();
	}

	FORCEINLINE void SetWhetherTouchedGroundOrWall(const bool b) { CanDash = b; }

	UFUNCTION(BlueprintPure)
	FORCEINLINE bool GetWhetherTouchedGroundOrWall() const { return CanDash; }

	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetMaxRunningSpeed() const { return MaxRunningSpeed; }
	FORCEINLINE float GetMinRunningSpeed() const { return RunningStateSpeedMinimum; }

	FORCEINLINE UCharacterStateMachine* GetCharacterStateMachine() const { return StateMachine; }
	FORCEINLINE USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	FORCEINLINE float GetCoyoteTime() const { return CoyoteTime; }

	AActor* GetLastInteractedWall() const { return LastInteractedWall; }
	void SetLastInteractedWall(AActor* HitResult) { LastInteractedWall = HitResult; }

private: //For mechanics
	UPROPERTY(VisibleAnywhere, Category = "Movement Settings", DisplayName= "Current Movement State")
	ECharacterMovementState CurrentMovementState = ECharacterMovementState::Idle;

	//UPROPERTY(EditAnywhere, Category = "Movement Settings|Movement|Walking")
	//UCurveFloat* WalkingAccelerationTime;
	
	//UPROPERTY(EditAnywhere, Category = "Movement Settings|Movement|Walking")
	//float PreciseWalkingSpeed;
	
	UPROPERTY(EditAnywhere, Category = "Movement Settings|Movement|Falling")
	UCurveFloat* PostFallAccelerationTime;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Movement|Falling")
	bool DisableExtraGravity = false;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Movement|Falling", meta=(Tooltip = "This will OVERRIDE gravity, not add to it."))
	float GravityWhileFalling;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Movement|Falling", meta = (ClampMin = 0))
	float FallZDistanceUnit;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Movement|Falling", meta = (ClampMin = 0, ClampMax = 1))
	float PenaltyMultiplierPerFallUnit;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Movement|Falling", meta = (ClampMin = 0, ClampMax = 1))
	float MaxPenaltyMultiplier;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Movement|Running")
	UCurveFloat* RunningAccelerationTime;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Movement|Running", meta = (ClampMin = 0))
	float RunningStateSpeedMinimum = 800;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Movement|Running", meta = (ClampMin = 0))
	float MaxRunningSpeed;

	UPROPERTY(EditAnywhere,  Category = "Movement Settings|Jump", meta = (ClampMin = 0, Tooltip = "Max time after a player can jump after platform"))
	float CoyoteTime;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Jump", meta = (ClampMin = 0))
	float JumpStrength;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Camera")
	float JitterSlowMinAngle;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Camera", meta = (ClampMin = 0, Tooltip = "At 1 Strength, it is 1% per 1 angle difference"))
	float JitterSlowPercentageStrength;

	UPROPERTY(EditAnywhere, Category= "Movement Settings|LookBack", meta = (ClampMin = 0, ClampMax = 1))
	UCurveFloat* LookBackTimeScale;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Debug")
	bool DebugVelocity;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Debug")
	bool DebugFall;

	UPROPERTY(EditAnywhere, Category = "Movement Settings|Debug")
	bool DebugCameraLeftRight;


	//Debug

public:
	UPROPERTY(VisibleAnywhere, Category = "Movement Settings|Debug")
	float AccelerationTimer = 0;

private:
	UPROPERTY(VisibleAnywhere, Category = "Movement Settings|Debug")
	float LookBackTimer = 0;
	UPROPERTY(VisibleAnywhere, Category = "Movement Settings|Debug")
	float CalculatedPostFallMultiplier;
	UPROPERTY(VisibleAnywhere, Category = "Movement Settings|Debug")
	float FallStartZ;
	UPROPERTY(VisibleAnywhere, Category = "Movement Settings|Debug")
	float FallDistance;
	UPROPERTY(VisibleAnywhere, Category = "Movement Settings|Debug")
	float PreviousFrameYaw = 0;

	UPROPERTY(VisibleAnywhere, Category = "Movement Settings|Debug")
	bool CanDash;
	UPROPERTY(VisibleAnywhere, Category = "Movement Settings|Debug")
	bool Falling = false;
	bool CanJump = true;

	float InternalCoyoteTimer = 0;
	FVector2d PreviousMovementVector;

	UPROPERTY()
	AActor* LastInteractedWall = nullptr;
	
};
