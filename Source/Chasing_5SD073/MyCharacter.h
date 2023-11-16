// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

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

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;

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

	
	bool moving = false;
	bool sprint = false;
	
	UPROPERTY(EditAnywhere, Category = "CustomValues")
	float slowPrecentage;
	
	UPROPERTY(EditAnywhere, Category = "CustomValues")
	float cameraJitter;
	
	UPROPERTY(EditAnywhere, Category = "CustomValues")
	float slideSpeedBoost;
	
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
	float gravityOrigin;

	UPROPERTY(EditAnywhere, Category = "CustomValues")
	float gravityLow;

	UPROPERTY(EditAnywhere, Category = "CustomValues")
	float slideSpeedMax;

	UPROPERTY(EditAnywhere, Category = "CustomValues")
	// when this timer reaches the max, player stops slide
	double slideTimer;
	
	float accelerationTimer;
	float fallingTimer;
	bool landed;
	bool dash;
	bool dashOnce;
	float airDashDelayTimer;
	bool startDelay;
	bool startDash;
	//float slideTime;
	bool boostSlide;

	UPROPERTY(VisibleAnywhere)
	bool setupSlidingTimer;

	UPROPERTY(VisibleAnywhere)
	double currenttimer = 0;
	
	double initialSlideTimer = 0;
	FVector dashValue;
	void DashAction();
	void SetupSlide();
	
	// Movements
	void Acceleration();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Slide();
	void LookBack();
	void LookFront();
	void AirDash(const FInputActionValue& Value);
	
	// Reset
	void SpeedReset();
	void ResetAfterSlide();

	// Raycast
	bool onSlope;
	void Ray();
	void GroundRaycast(float DeltaTime);
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

	float getCurrentAccelerationRate;

	UPROPERTY(EditAnywhere, Category = "CustomValues")
	FVector slideBoost;
	bool fallSliding;
};
