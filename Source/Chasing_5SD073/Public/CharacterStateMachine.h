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
//todo rewrite and shorten

class UStateComponentBase;

UENUM(BlueprintType)
enum class ECharacterState : uint8
{
	DefaultState,
	Sliding,
	WallClimbing,
	WallRunning,
	AirDashing,
	// Add other states as needed
};

USTRUCT(BlueprintType)
struct FMechanicStateData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	ECharacterState State;
	
	UPROPERTY(VisibleAnywhere)
	UStateComponentBase* Component;

	// Default constructor
	FMechanicStateData(): State(ECharacterState::DefaultState), Component(nullptr)
	{
	}

	FMechanicStateData(const ECharacterState& InState, UStateComponentBase* InComponent)
		: State(InState), Component(InComponent)
	{}
};

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

	//This switches states. Returns true if successful
	UFUNCTION(BlueprintCallable)
	bool SetState(const ECharacterState& NewStateEnum);
	void UpdateStateMachine();
	void ManualExitState();
	void DetectStates();
	void SetupStateMachine();

	void OverrideMovementInput(FVector2d& NewMovementVector);
	void OverrideAcceleration(float& NewSpeed);
	void OverrideCameraInput(FVector2d& NewRotationVector);
	void OverrideDebug() const;

	bool IsThisCurrentState(const UStateComponentBase& Component) const { return CurrentState == &Component; }
	bool IsCurrentStateNull() const { return CurrentState == nullptr; }

	
	UStateComponentBase* GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintPure)
	FORCEINLINE ECharacterState GetCurrentEnumState() const { return CurrentEnumState; }

private:
	static FString EnumToString(const ECharacterState& ToConvert);
	void CheckForDuplicates();
	void GetComponentReferences(const TArray<ECharacterState>& HierarchyArray);
	UStateComponentBase* TranslateEnumToState(const ECharacterState& Enum) const;

	//Add quick debug text with red color and 0 lifetime
	static void DebugText(const FString& Text);

	UPROPERTY(EditAnywhere, Category= "Character State Machine")
	TArray<ECharacterState> MechanicsHierarchy;
	
	UPROPERTY(VisibleAnywhere, Category= "Character State Machine", DisplayName= "Current State")
	ECharacterState CurrentEnumState = ECharacterState::DefaultState;

	UPROPERTY(EditAnywhere, Category= "Character State Machine|Debug")
	bool DebugStateMachine = false;
	
	UPROPERTY(VisibleAnywhere,Category= "Character State Machine|Debug", DisplayName= "Current State Internal")
	UStateComponentBase* CurrentState = nullptr;
	
	UPROPERTY(VisibleAnywhere, Category= "Character State Machine|Debug")
	TArray<FMechanicStateData> MechanicsList;

	
	bool RunUpdate = false;
};

