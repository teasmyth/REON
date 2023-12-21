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
	virtual void OnEnterState(UCharacterStateMachine& SM) override;
	virtual void OnUpdateState(UCharacterStateMachine& SM) override;
	virtual void OnExitState(UCharacterStateMachine& SM) override;

	virtual void OverrideMovementInput(FVector2d& NewMovementVector) override;
	virtual void OverrideCameraInput(UCameraComponent& Camera, FVector2d& NewRotationVector) override;

	UPROPERTY(EditAnywhere, Category= "Settings")
	bool bDebug = false;

	UPROPERTY(EditAnywhere, Category= "Settings")
	UCurveFloat* WallRunGravityCurve;

	//UPROPERTY(EditAnywhere, Category= "Settings", meta = (Tooltip = "The tempory max speed of the player. If 0, it will use max running speed"))
	//float TemporaryMaxSpeedDuringWallRun;

private:
	EWallOrientation WallOrientation = None;

	FHitResult HitResult;
	AActor* PreviousWall = nullptr;
	float InternalTimer;
	float InternalGravityScale;

	void RotatePlayerAlongsideWall(const FHitResult& Hit);
	bool CheckWhetherStillWallRunning();
};
