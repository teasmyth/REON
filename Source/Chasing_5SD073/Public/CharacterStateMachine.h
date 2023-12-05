// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateComponentBase.h"
#include "Components/ActorComponent.h"
#include "CharacterStateMachine.generated.h"




UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CHASING_5SD073_API UCharacterStateMachine : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UCharacterStateMachine();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	ECharacterState CurrentEnumState = ECharacterState::Idle;
	UPROPERTY()
	UStateComponentBase* CurrentState = nullptr;

	UPROPERTY()
	UStateComponentBase* Sliding;

	UPROPERTY()
	UStateComponentBase* WallRunning;

	UPROPERTY()
	UStateComponentBase* WallClimbing;

	UPROPERTY()
	UStateComponentBase* AirDashing;

	UPROPERTY()
	UStateComponentBase* Falling;

	void SetState(const ECharacterState& NewStateEnum);

private:
	//Internal check whether the player can switch from current state to the new one.
	void SetUpStates();
	UStateComponentBase* TranslateEnumToState(const ECharacterState& Enum) const;
	bool RunUpdate;
};
