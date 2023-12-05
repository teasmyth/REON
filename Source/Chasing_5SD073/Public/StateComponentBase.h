// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StateComponentBase.generated.h"

//Using custom enum types and translating back and forth because I cannot make UInterface as making it would make it a UObject,
//which cannot be inherited by other Actor Components. Hence the translation layer, which is also visible to BPs.
//Instead, I am using an actor component base class (to have shared functionality) that all the other states inherit.
//another reason is I am using bunch of switch statement which requires constants. Afaik, switching based on classes (if i were to use CurrentState
//as a class, rather than an enum) wouldn't work. So, that is another why I am using enums for switching between states.

UENUM(BlueprintType)
enum class ECharacterState : uint8
{
	Idle,
	Walking,
	Running,
	Sliding,
	WallClimbing,
	WallRunning,
	AirDashing,
	Falling
	// Add other states as needed
};

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

	UPROPERTY(EditFixedSize, EditAnywhere, Category = "Settings", meta = (ToolTip = "The list describes TO which states this state can transtion"))
	TMap<ECharacterState, bool> PossibleTransitions;
	
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
