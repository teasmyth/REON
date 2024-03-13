// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateComponentBase.h"
#include "Components/ActorComponent.h"
#include "AirDashingStateComponent.generated.h"

typedef TPair<FVector, FVector> Line;

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
	virtual bool OnSetStateConditionCheck(UCharacterStateMachine& SM) override;
	virtual void OnEnterState(UCharacterStateMachine& SM) override;
	virtual void OnUpdateState(UCharacterStateMachine& SM) override;
	virtual void OnExitState(UCharacterStateMachine& SM) override;
	
	virtual void OverrideMovementInput(UCharacterStateMachine& SM, FVector2d& NewMovementVector) override;
	virtual void OverrideDebug() override;

private:
	TArray<Line> GetEdges(const FBox& Bounds) const;
	void AddSlide();
	
private:
	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ClampMin = 0))
	float AirDashDistance = 0;

	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ToolTip = "The duration of the 'blink'", ClampMin = 0))
	float AirDashTime = 0;

	UPROPERTY(EditAnywhere, Category= "Edge Correction", meta = (ToolTip = "Enable edge correction debug"))
	bool EnableEdgeCorrectionDebug = true;

	UPROPERTY(EditAnywhere, Category= "Edge Correction", meta = (ToolTip = "Min distance from an edge", ClampMin = 0))
	float EdgeCorrectionAmount = 2.0f;
	
	UPROPERTY(EditAnywhere, Category= "Edge Correction", meta = (ToolTip = "Min distance from an edge", ClampMin = 0))
	float EdgeDistThreshold = 10.0f;

	UPROPERTY(EditAnywhere, Category= "Edge Correction", meta = (ToolTip = "The time it takes to perform edge correction", ClampMin = 0))
	float InterpSpeed = 0.05f;
	
	//Internal
	FVector InitialForwardVector;
	double InternalTimer;

	bool SweepOnDash = true;
	bool IsHoldingW = false;
	float HorizontalVelocity = 0.0f;
	float AccelerationTimer = 0.0f;

	FTimerHandle SlideTimerHandle;
	FTimerHandle ImpulseTimerHandle;
};
