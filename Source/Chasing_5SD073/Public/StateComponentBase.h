// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CharacterStateMachine.h"
#include "Components/ActorComponent.h"
#include "StateComponentBase.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CHASING_5SD073_API UStateComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UStateComponentBase();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:

	UPROPERTY(EditFixedSize, EditAnywhere, Category = "Settings", meta = (ToolTip = "The list describes FROM which states this state can transtion"))
	TMap<ECharacterState, bool> CanTransitionFromStateList;
	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//This is executed at the beginning of the state change. Do not completely override, leave the base.
	virtual void OnEnterState();

	//This is constantly executed during the state between OnEnterStateEvent and OnExitStateEvent. Do not completely override, leave the base.
	virtual void OnUpdateState();

	//This is executed at the ending of the state. Do not completely override, leave the base.
	virtual void OnExitState();

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
};
