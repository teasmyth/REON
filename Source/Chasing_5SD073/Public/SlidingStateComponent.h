// Fill out your copyright notice in the Description page of Project Sliding Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateComponentBase.h"
#include "Components/ActorComponent.h"
#include "SlidingStateComponent.generated.h"



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
	virtual void OnEnterState(UCharacterStateMachine& SM) override;
	virtual void OnUpdateState(UCharacterStateMachine& SM) override;
	virtual void OnExitState(UCharacterStateMachine& SM) override;

	virtual void OverrideMovement(FVector2d& NewMovementVector) override;
	virtual void OverrideCamera(UCameraComponent& Camera, FVector2d& NewRotationVector) override;

private:
	UPROPERTY(EditAnywhere, Category= "Sliding Settings", meta = (Tooltip = "Max Slide Duration (seconds)"))
	float MaxSlideDuration = 0;

	UPROPERTY(EditAnywhere, Category= "Sliding Settings", meta = (ToolTip = "Maximum possible speed is running max + speed boost"))
	float SlidingSpeedBoost = 0;

	UPROPERTY(EditAnywhere, Category= "Sliding Settings", meta = (ToolTip = "It is a % value, limiting the intensity of left right input."))
	float SlidingLeftRightMovementModifier = 0;

	UPROPERTY(EditAnywhere, Category= "Sliding Settings", meta = (ToolTip = "It is a % value, limiting the intensity of camera input. (all directions)"))
	float SlidingCameraModifier = 0;

	float InternalTimerStart;

	bool IsSlidingSetup = false;
	UCameraComponent* UsedCamera = nullptr;

	float CameraFullHeight = 0;
	float CameraReducedHeight = 0;
};
