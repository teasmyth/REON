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
	Running
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
	float fallingTimer;
	
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
	void AirDash(const FInputActionValue& Value);
	void DashAction();

	void SetCharacterSpeed(const float& NewSpeed) const;
	void AddCharacterSpeed(const float& NewSpeed) const;


	void WallMechanicsCheck();

	void ResetCharacterState();
	void MovementStateCheck();
	
	// Reset
	void SpeedReset();

	// Raycast
	bool onSlope;
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
	
	void DebugSpeed();
	void DebugLanding();
	void DebugSize();

private: //My stuff


	//Movement
	UPROPERTY(VisibleAnywhere, DisplayName= "Current Movement State")
	EMovementState CurrentMovementState = EMovementState::Idle;
	
	UPROPERTY(EditAnywhere, Category = "Movement Setting")
	UCurveFloat* WalkingAccelerationTime;

	UPROPERTY(EditAnywhere, Category = "Movement Setting")
	UCurveFloat* RunningAccelerationTime;
	
	UPROPERTY(EditAnywhere, Category = "Movement Setting")
	float RunningStateSpeedMinimum = 800;

	UPROPERTY(EditAnywhere, Category = "Movement Setting")
	float MaxRunningSpeed;

	UPROPERTY(EditAnywhere, Category = "Movement Setting", meta = (Tooltip = "This is the maximum possible speed with any kind of bonus combined."))
	float MaxPossibleSpeed;
};
