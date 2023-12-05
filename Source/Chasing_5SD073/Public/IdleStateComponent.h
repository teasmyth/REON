// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateComponentBase.h"
#include "Components/ActorComponent.h"
#include "IdleStateComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CHASING_5SD073_API UIdleStateComponent : public UStateComponentBase
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UIdleStateComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnEnterState() override;
	virtual void OnUpdateState() override;
	virtual void OnExitState() override;

		
};
