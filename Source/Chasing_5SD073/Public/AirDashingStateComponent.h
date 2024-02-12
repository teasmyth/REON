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
	virtual bool OnSetStateConditionCheck(UCharacterStateMachine& SM) override;
	virtual void OnEnterState(UCharacterStateMachine& SM) override;
	virtual void OnUpdateState(UCharacterStateMachine& SM) override;
	virtual void OnExitState(UCharacterStateMachine& SM) override;
	
	virtual void OverrideMovementInput(UCharacterStateMachine& SM, FVector2d& NewMovementVector) override;
	virtual void OverrideDebug() override;

private:
	void AddSlide();
	
private:
	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ClampMin = 0))
	float AirDashDistance = 0;

	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ToolTip = "The duration of the 'blink'", ClampMin = 0))
	float AirDashTime = 0;


	FTimerHandle MemberTimerHandle;
	
	//Internal
	FVector InitialForwardVector;
	double InternalTimer;

	bool IsHoldingW = false;
	float HorizontalVelocity = 0.0f;
};
