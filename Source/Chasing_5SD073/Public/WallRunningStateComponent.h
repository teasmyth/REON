// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateComponentBase.h"
#include "Components/ActorComponent.h"
#include "WallRunningStateComponent.generated.h"

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
	
private:
	EWallOrientation WallOrientation = None;

	FHitResult PrevResult;
	FHitResult HitResult;
	FHitResult EmptyResult; 
	float GravityTimer;
	float InternalGravityScale;

	void RotatePlayerAlongsideWall(const FHitResult& Hit) const;
	bool CheckWhetherStillWallRunning();
	void DetectWallRun();

	UPROPERTY(EditAnywhere, Category= "Settings",
		meta = (Tooltip = "This stops Wall Run sensors from being an insta trigger, preventing accidentals."))
	float WallRunTriggerDelay;
	float TriggerTimer;

	UPROPERTY(EditAnywhere, Category= "Settings")
	UCurveFloat* WallRunGravityCurve;

	UPROPERTY(EditAnywhere, Category = "Settings")
	float WallCheckDistance;

	UPROPERTY(EditAnywhere, Category = "Settings")
	float WallRunSideAngle;
};
