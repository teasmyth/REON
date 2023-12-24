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
	
	virtual void OverrideMovementInput(UCharacterStateMachine& SM, FVector2d& NewMovementVector) override;
	virtual void OverrideDebug() override;
	
	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ToolTip = ""))
	float AirDashDistance = 0;

	UPROPERTY(EditAnywhere, Category= "Settings", meta = (ToolTip = "The duration of the 'blink'"))
	float AirDashTime = 0;
	
private:
	FVector InitialForwardVector;
	double InternalTimer;
	
};
