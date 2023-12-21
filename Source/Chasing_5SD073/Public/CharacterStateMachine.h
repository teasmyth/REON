// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CharacterStateMachine.generated.h"

//Using custom enum types and translating back and forth because I cannot make UInterface as making it would make it a UObject,
//which cannot be inherited by other Actor Components. Hence the translation layer, which is also visible to BPs.
//Instead, I am using an actor component base class (to have shared functionality) that all the other states inherit.
//another reason is I am using bunch of switch statement which requires constants. Afaik, switching based on classes (if i were to use CurrentState
//as a class, rather than an enum) wouldn't work. So, that is another why I am using enums for switching between states.

UENUM(BlueprintType)
enum class ECharacterState : uint8
{
	DefaultState,
	Sliding,
	WallClimbing,
	WallRunning,
	AirDashing,
	Falling
	// Add other states as needed
};

class UStateComponentBase;

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


	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, DisplayName= "Current State")
	ECharacterState CurrentEnumState = ECharacterState::DefaultState;

	UPROPERTY()
	UStateComponentBase* CurrentState = nullptr;


	//TODO research a more automated thing for this, instead of manually adding stuff.


	UPROPERTY()
	UStateComponentBase* DefaultState = nullptr;

	UPROPERTY()
	UStateComponentBase* Sliding = nullptr;

	UPROPERTY()
	UStateComponentBase* WallRunning = nullptr;

	UPROPERTY()
	UStateComponentBase* WallClimbing = nullptr;

	UPROPERTY()
	UStateComponentBase* AirDashing = nullptr;

	UPROPERTY()
	UStateComponentBase* Falling = nullptr;

	void SetState(const ECharacterState& NewStateEnum);

	//This is used for manual OnExit.
	void ManualExitState();

	bool IsCurrentStateNull() const { return CurrentState == nullptr; }
	UStateComponentBase* GetCurrentState() const { return CurrentState; } // make the private and use this instead this
	ECharacterState GetCurrentEnumState() const { return CurrentEnumState; } // make the private and use this instead this

private:
	void SetupStates();
	//Internal check whether the player can switch from current state to the new one.
	UStateComponentBase* TranslateEnumToState(const ECharacterState& Enum) const;
	bool RunUpdate = false;
};
