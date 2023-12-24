// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <string>

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MyCharacter.generated.h"

class ICharacterState;
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

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FrontCam;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* BackCam;

	UPROPERTY()
	UCharacterStateMachine* StateMachine = nullptr;

public:
	// Sets default values for this character's properties
	AMyCharacter();

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* AirDashAction;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "CustomInputs")
	class UInputAction* SlideAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CustomInputs")
	class UInputAction* LookBackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FrontCam; }

	// custom values
	UPROPERTY(EditAnywhere, Category = "CustomValues")
	float slowPrecentage;

	UPROPERTY(EditAnywhere, Category = "CustomValues")
	float cameraJitter;


	UPROPERTY(EditAnywhere, Category = "CustomValues")
	float accelerationSpeedRate;

	UPROPERTY(EditAnywhere, Category = "CustomValues")
	float maxFallingPenaltyTime;

	UPROPERTY(EditAnywhere, Category = "CustomValues")
	float maxFallingSpeedSlowPenalty;

	UPROPERTY(EditAnywhere, Category = "CustomValues")
	float airDashDistance;

	UPROPERTY(EditAnywhere, Category = "CustomValues")
	// Delay timer
	float airDashDelay;

	UPROPERTY(EditAnywhere, Category = "CustomValues")
	// normal gravity 
	float gravityOrigin;

	UPROPERTY(EditAnywhere, Category = "CustomValues")
	// ur gravity is low when u want to air dash
	float gravityLow;


	// accelerate
	float accelerationTimer;


	bool landed;

	// airdash
	bool dash;
	bool dashOnce;
	float airDashDelayTimer;
	bool startDelay;
	bool startDash;

	bool boostSlide;


	double initialSlideTimer = 0;
	FVector dashValue; // how far u airDash

	float getCurrentAccelerationRate;

	bool fallSliding;

	// Movements functions
	void Acceleration(const float& DeltaTime);
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Slide();
	void LookBack();
	void LookFront();
	void JumpAndDash();


	void ResetDash() { TouchedGroundOrWall = true; }
	bool IsFalling() const;

	void ResetSlide();
	void MovementStateCheck();


	// Raycast
	void GroundRaycast();
	void SliderRaycast();

	// Debug
	// Print on screen
	UPROPERTY(EditAnywhere, Category = "Debugging")
	bool debugSpeed;

	// Debug log
	UPROPERTY(EditAnywhere, Category = "Debugging")
	bool debugLanding;

	UPROPERTY(EditAnywhere, Category = "Debugging")
	bool debugGroundRaycast;

	UPROPERTY(EditAnywhere, Category = "Debugging")
	bool debugSlideRaycast;

	UPROPERTY(EditAnywhere, Category = "Debugging")
	bool bDebugWallMechanics;

	void DebugSpeed();
	void DebugLanding();
	void DebugSize();

	UFUNCTION(BlueprintPure)
	EMovementState GetCharacterMovementState() const { return CurrentMovementState; }
	FORCEINLINE float GetVerticalVelocity() const { return FVector2d(GetCharacterMovement()->Velocity.X, GetCharacterMovement()->Velocity.Y).Size(); }
	FORCEINLINE void SetWhetherTouchedGroundOrWall(const bool b) { TouchedGroundOrWall = b; }
	FORCEINLINE bool GetWhetherTouchedGroundOrWall() const { return TouchedGroundOrWall; }
	FORCEINLINE float GetMaxRunningSpeed() const { return MaxRunningSpeed; }
	FORCEINLINE UCharacterStateMachine* GetCharacterStateMachine() const { return StateMachine; }

private:
	//Movement
	UPROPERTY(VisibleAnywhere, Category = "Movement Settings", DisplayName= "Current Movement State")
	EMovementState CurrentMovementState = EMovementState::Idle;

	UPROPERTY(EditAnywhere, Category = "Movement Settings")
	UCurveFloat* WalkingAccelerationTime;

	UPROPERTY(EditAnywhere, Category = "Movement Settings")
	UCurveFloat* PostFallAccelerationTime;

	UPROPERTY(EditAnywhere, Category = "Movement Settings")
	float MinimumFallTime;

	UPROPERTY(EditAnywhere, Category = "Movement Settings")
	float PenaltyMultiplierPerSecond;

	UPROPERTY(EditAnywhere, Category = "Movement Settings")
	float MaxPenaltyMultiplier;

	UPROPERTY(EditAnywhere, Category = "Movement Settings")
	UCurveFloat* RunningAccelerationTime;

	UPROPERTY(EditAnywhere, Category = "Movement Settings")
	float RunningStateSpeedMinimum = 800;

	UPROPERTY(EditAnywhere, Category = "Movement Settings")
	float MaxRunningSpeed;

	UPROPERTY(EditAnywhere, Category = "Movement Settings")
	float JumpStrength;

	bool TouchedGroundOrWall;
	float CalculatedPostFallMultiplier;
	float FallingTimer = 0;
};
