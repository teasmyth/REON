// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateComponentBase.h"
#include "Components/ActorComponent.h"
#include "WallClimbingStateComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CHASING_5SD073_API UWallClimbingStateComponent : public UStateComponentBase
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UWallClimbingStateComponent();

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


private:

	bool CheckLedge() const;
	bool CheckLeg() const;
	
	UPROPERTY(EditAnywhere, Category= "Settings")
	bool Debug;

	UPROPERTY(EditAnywhere, Category= "Settings")
	float WallClimbSpeed;
	
	UPROPERTY(EditAnywhere, Category= "Settings")
	float LedgeGrabCheckZOffset;

	UPROPERTY(EditAnywhere, Category= "Settings")
	float LedgeGrabCheckDistance;

	UPROPERTY(EditAnywhere, Category= "Settings")
	float MaxWallClimbDuration;
	

	float InternalTimer;
};
