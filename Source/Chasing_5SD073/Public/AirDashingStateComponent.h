// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateComponentBase.h"
#include "Components/ActorComponent.h"
#include "AirDashingStateComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CHASING_5SD073_API UAirDashingStateComponent : public UStateComponentBase
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAirDashingStateComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnEnterState(UCharacterStateMachine& SM) override;
	virtual void OnUpdateState(UCharacterStateMachine& SM) override;
	virtual void OnExitState(UCharacterStateMachine& SM) override;
	
	virtual void OverrideMovement(float& NewSpeed) override;
	virtual void OverrideMovementInputSensitivity(FVector2d& NewMovementVector) override;

	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ToolTip = ""))
	float AirDashSpeedBoost = 0;

	
	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ToolTip = ""))
	float AirDashDistance = 0;
	FVector InitialLocation;

	bool bIsDashing = false;
	bool bDashed = false;
	double InternalTimerStart;
	float DashTimeFrame = 1;

	FVector CurrentVelocity;
};
