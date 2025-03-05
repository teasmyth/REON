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

	UPROPERTY(BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FrontCam;

	UPROPERTY(BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* BackCam;

	UPROPERTY()
	UCharacterStateMachine* StateMachine = nullptr;

	UPROPERTY(EditAnywhere, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* NoJumpAction;

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

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Input, meta = (AllowPrivateAccess = "true"))
	FVector2D Sensitivity = FVector2d(0.5f, 0.5f);

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
	void NoJumpInput();
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
	void ResetJump() { PlayerCanJump = true; }
	void DisableJump() {PlayerCanJump = false;}
	virtual void Jump() override { JumpAndDash(); }

	void ResetFalling()
	{
		Falling = false;
		FallDistance = -1;
		FallStartZ = GetActorLocation().Z;
		InternalFallingTimer = 0;
	}

	void ResetSlide();

	UFUNCTION(BlueprintCallable)
	FORCEINLINE void AddEnergy(const float Energy) { CurrentEnergy = FMath::Clamp(CurrentEnergy + Energy, 0.0f, MaxEnergy); }

	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetEnergy() const { return CurrentEnergy; }

	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetMaxEnergy() const { return MaxEnergy; }

	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetEnergyPercentage() const { return FMath::Clamp(CurrentEnergy / MaxEnergy, 0, 1); }

	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetStartingEnergy() const { return  FMath::Clamp(StartingEnergy / MaxEnergy, 0, 1); }

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

	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetMinRunningSpeed() const { return RunningStateSpeedMinimum; }

	FORCEINLINE UCharacterStateMachine* GetCharacterStateMachine() const { return StateMachine; }
	FORCEINLINE USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	FORCEINLINE float GetCoyoteTime() const { return CoyoteTime; }

	AActor* GetLastInteractedWall() const { return LastInteractedWall; }
	void SetLastInteractedWall(AActor* HitResult) { LastInteractedWall = HitResult; }

private: //For mechanics
	UPROPERTY(VisibleAnywhere, Category = "REON Player Settings", DisplayName= "Current Movement State")
	ECharacterMovementState CurrentMovementState = ECharacterMovementState::Idle;

	//UPROPERTY(EditAnywhere, Category = "REON Player Settings|Movement|Walking")
	//UCurveFloat* WalkingAccelerationTime;
	
	//UPROPERTY(EditAnywhere, Category = "REON Player Settings|Movement|Walking")
	//float PreciseWalkingSpeed;

	UPROPERTY(EditAnywhere, Category="REON Player Settings|Energy")
	float MaxEnergy;
	UPROPERTY(EditAnywhere, Category="REON Player Settings|Energy")
	float StartingEnergy;
	UPROPERTY(EditAnywhere, Category="REON Player Settings|Energy")
	float CurrentEnergy;
	UPROPERTY(EditAnywhere, Category="REON Player Settings|Energy")
	float GenerateEnergyPerSecond;
	UPROPERTY(VisibleAnywhere, Category="REON Player Settings|Energy")
	bool CurrentStateDrainedEnergy = false;
	
	
	
	UPROPERTY(EditAnywhere, Category = "REON Player Settings|Movement|Falling")
	UCurveFloat* PostFallAccelerationTime;

	UPROPERTY(EditAnywhere, Category = "REON Player Settings|Movement|Falling")
	bool DisableExtraGravity = false;

	UPROPERTY(EditAnywhere, Category = "REON Player Settings|Movement|Falling", meta=(Tooltip = "This will OVERRIDE gravity, not add to it."))
	float GravityWhileFalling;

	UPROPERTY(EditAnywhere, Category = "REON Player Settings|Movement|Falling", meta =(Tooltip = "After this period of time, the fall penalty ends, instead of reaching minimum speed."))
	float FallPenaltyTimer = 1.5f;

	UPROPERTY(EditAnywhere, Category = "REON Player Settings|Movement|Falling", meta = (ClampMin = 0))
	float FallZDistanceUnit;

	UPROPERTY(EditAnywhere, Category = "REON Player Settings|Movement|Falling", meta = (ClampMin = 0, ClampMax = 1))
	float PenaltyMultiplierPerFallUnit;

	UPROPERTY(EditAnywhere, Category = "REON Player Settings|Movement|Falling", meta = (ClampMin = 0, ClampMax = 1))
	float MaxPenaltyMultiplier;

	UPROPERTY(EditAnywhere, Category = "REON Player Settings|Movement|Running")
	UCurveFloat* RunningAccelerationTime;

	UPROPERTY(EditAnywhere, Category = "REON Player Settings|Movement|Running", meta = (ClampMin = 0))
	float RunningStateSpeedMinimum = 800;

	UPROPERTY(EditAnywhere, Category = "REON Player Settings|Movement|Running", meta = (ClampMin = 0))
	float MaxRunningSpeed;

	UPROPERTY(EditAnywhere,  Category = "REON Player Settings|Jump", meta = (ClampMin = 0, Tooltip = "Max time after a player can jump after platform"))
	float CoyoteTime;

	UPROPERTY(EditAnywhere, Category = "REON Player Settings|Jump", meta = (ClampMin = 0))
	float JumpStrength;

	UPROPERTY(EditAnywhere, Category = "REON Player Settings|Camera")
	float JitterSlowMinAngle;

	UPROPERTY(EditAnywhere, Category = "REON Player Settings|Camera", meta = (ClampMin = 0, Tooltip = "At 1 Strength, it is 1% per 1 angle difference"))
	float JitterSlowPercentageStrength;

	UPROPERTY(EditAnywhere, Category= "REON Player Settings|LookBack", meta = (ClampMin = 0, ClampMax = 1))
	UCurveFloat* LookBackTimeScale;

	UPROPERTY(EditAnywhere, Category = "REON Player Settings|Debug")
	bool DebugVelocity;

	UPROPERTY(EditAnywhere, Category = "REON Player Settings|Debug")
	bool DebugFall;

	UPROPERTY(EditAnywhere, Category = "REON Player Settings|Debug")
	bool DebugCameraLeftRight;


	//Debug

public:
	UPROPERTY(VisibleAnywhere, Category = "REON Player Settings|Debug")
	float AccelerationTimer = 0;

private:
	UPROPERTY(VisibleAnywhere, Category = "REON Player Settings|Debug")
	float LookBackTimer = 0;
	UPROPERTY(VisibleAnywhere, Category = "REON Player Settings|Debug")
	float CalculatedPostFallMultiplier;
	UPROPERTY(VisibleAnywhere, Category = "REON Player Settings|Debug")
	float FallStartZ;
	UPROPERTY(VisibleAnywhere, Category = "REON Player Settings|Debug")
	float FallDistance;
	UPROPERTY(VisibleAnywhere, Category = "REON Player Settings|Debug")
	float PreviousFrameYaw = 0;

	UPROPERTY(VisibleAnywhere, Category = "REON Player Settings|Debug")
	bool CanDash;
	UPROPERTY(VisibleAnywhere, Category = "REON Player Settings|Debug")
	bool Falling = false;
	UPROPERTY(VisibleAnywhere, Category = "REON Player Settings|Debug")
	bool PlayerCanJump = true;
	float InternalFallingTimer = 0;

	float InternalCoyoteTimer = 0;
	FVector2d PreviousMovementVector;

	UPROPERTY()
	AActor* LastInteractedWall = nullptr;
	
};
