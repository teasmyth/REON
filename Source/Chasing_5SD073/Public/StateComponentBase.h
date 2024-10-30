// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Chasing_5SD073/MyCharacter.h"
#include "Components/ActorComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "StateComponentBase.generated.h"

class UCharacterStateMachine;


UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class CHASING_5SD073_API UStateComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UStateComponentBase();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY()
	UCapsuleComponent* PlayerCapsule = nullptr;

	UPROPERTY()
	UCharacterMovementComponent* PlayerMovement = nullptr;

	UPROPERTY()
	AMyCharacter* PlayerCharacter = nullptr;

	UPROPERTY(EditFixedSize, EditAnywhere, Category = "Settings|General Settings",
		meta = (ToolTip = "The list describes FROM which states this state can transtion"))
	TMap<ECharacterState, bool> CanTransitionFromStateList;

	UPROPERTY(EditAnywhere, Category= "Settings|General Settings") //Dont add space after general!
	bool CountTowardsFalling = false;

	UPROPERTY(EditAnywhere, Category= "Settings|General Settings")
	bool ResetsDash = false;

	UPROPERTY(EditAnywhere, Category= "Settings|General Settings")
	bool ResetsJump = true;

	UPROPERTY(EditAnywhere, Category = "Settings|General Settings")
	bool DebugMechanic = false;

	UPROPERTY(EditAnywhere, Category = "Settings|General Settings")
	FColor DebugColor;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//This runs before switching state, letting states access player variables before 'mechanically' entering a state.
	//For example checking whether player is on ground. If returns false, switching state is aborted and the current state does not exit.
	virtual bool OnSetStateConditionCheck(UCharacterStateMachine& SM);

	//Will need to make a custom event which can be used with BP version of the state machine. I cannot override a function in BP
	//In a way, that I can modify the return parameter (which is needed for onsetsateconditioncheck(). However, I believe
	//In a fully BP tailored version of SM, it should be possible.
	
	//This is executed at the beginning of the state change. Do not completely override, leave the base.
	virtual void OnEnterState(UCharacterStateMachine& SM);

	//This is constantly executed during the state between OnEnterStateEvent and OnExitStateEvent. Do not completely override, leave the base.
	virtual void OnUpdateState(UCharacterStateMachine& SM);

	//This is executed at the ending of the state. Do not completely override, leave the base.
	virtual void OnExitState(UCharacterStateMachine& SM);

	virtual void OverrideMovementInput(UCharacterStateMachine& SM, FVector2d& NewMovementVector);
	
	virtual void OverrideNoMovementInputEvent(UCharacterStateMachine& SM);

	virtual void OverrideAcceleration(UCharacterStateMachine& SM, float& NewSpeed);

	virtual void OverrideCameraInput(UCharacterStateMachine& SM, FVector2d& NewRotationVector);

	virtual void OverrideJump(UCharacterStateMachine& SM, FVector& JumpVector);

	virtual void OverrideNoJump(UCharacterStateMachine& SM);

	//This for mechanics that require automated triggers rather than manual one. State machine will make sure a mechanic will not try to detect itself
	//Or if the mechanic prohibits transitioning from the current state.
	virtual void OverrideDetectState(UCharacterStateMachine& SM);

	//This runs in Debug
	virtual void OverrideDebug();

	bool DoesItCountTowardsFalling() const { return CountTowardsFalling; }
	bool DoesItResetDash() const { return ResetsDash; }
	bool GetDebugMechanic() const { return DebugMechanic; }
	TMap<ECharacterState, bool> GetTransitionList() const { return CanTransitionFromStateList; }

	bool GetCanExitState() const { return CanExitState; }
protected:
	//Helper Methods

	//Line Trace Single Channel, used when HitResult is needed. Return true if there is a hit. Automatically ignores Owner and uses ECC_Visibility.
	bool LineTraceSingle(FHitResult& HitR, const FVector& Start, const FVector& End) const;
	//Line Trace Single Channel, used when HitResult is not needed. Return true if there is a hit. Automatically ignores Owner and uses ECC_Visibility.
	bool LineTraceSingle(const FVector& Start, const FVector& End) const;
	
	static FVector RotateVector(const FVector& VectorToRotate, const FRotator& RotationDegreesPerAxis, const float LengthOverride = 1);

	bool IsKeyDown(const FKey& Key) const;


	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStateEnterDelegate);

	//This is executed at the beginning of the state change. Note: this runs before mechanical execution, meaning the BP event will run first.
	UPROPERTY(BlueprintAssignable, DisplayName= "On Enter Event")
	FOnStateEnterDelegate OnEnterStateDelegate;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpdateStateDelegate);

	//This is constantly executed during the state between OnEnterStateEvent and OnExitStateEvent. Note: this runs before mechanical execution, meaning the BP event will run first.
	UPROPERTY(BlueprintAssignable, DisplayName= "On Update Event")
	FOnUpdateStateDelegate OnUpdateStateDelegate;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnExitStateDelegate);

	//This is executed at the ending of the state. Note: this runs before mechanical execution, meaning the BP event will run first.
	UPROPERTY(BlueprintAssignable, DisplayName= "On Exit Event")
	FOnExitStateDelegate OnExitStateDelegate;

	UPROPERTY(EditAnywhere, Category = "Settings|General Settings")
	bool CanExitState = true;
};
