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

	virtual void OverrideMovementInput(UCharacterStateMachine& SM, FVector2d& NewMovementVector) override;
	virtual void OverrideCameraInput(UCharacterStateMachine& SM, FVector2d& NewRotationVector) override;

	UPROPERTY(EditAnywhere, Category= "Settings")
	bool bDebug = false;

	UPROPERTY(EditAnywhere, Category= "Settings")
	UCurveFloat* WallRunGravityCurve;


private:
	EWallOrientation WallOrientation = None;

	FHitResult HitResult;
	float InternalTimer;
	float InternalGravityScale;

	void RotatePlayerAlongsideWall(const FHitResult& Hit) const;
	bool CheckWhetherStillWallRunning();
};
