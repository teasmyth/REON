// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NavSystemVolumeManager.generated.h"

UCLASS()
class CHASING_5SD073_API ANavSystemVolumeManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANavSystemVolumeManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void SetUpNavVolumes();

	void SetUpTarget();

	//Call this if volume manager loses reference to the Target, ie.: Target has been killed/destroyed.
	UFUNCTION(BlueprintCallable, Category = "Navigation System Manager")
	void SetUpTarget(AActor* NewTarget);

	void SetUpAI_Agent();

	//Call this if volume manager loses reference to the AI Agent, ie.: Agent has been killed/destroyed.
	UFUNCTION(BlueprintCallable, Category = "Navigation System Manager")
	void SetUpAI_Agent(AActor* NewAgent);

	UFUNCTION(BlueprintPure, Category = "Navigation System Manager")
	AActor* GetTargetActor() const { return TargetActor;}

	UFUNCTION(BlueprintPure, Category = "Navigation System Manager")
	AActor* GetAI_AgentActor() const { return AI_AgentActor;}

private:
	UPROPERTY(EditAnywhere, Category= "Navigation System Manager")
	AActor* TargetActor = nullptr;

	UPROPERTY(EditAnywhere, Category= "Navigation System Manager")
	AActor* AI_AgentActor = nullptr;

	//Sets how many seconds after the Agent leaves the grid starts unloading the grid. If the grid is reentered within this frame, it interrupts the unloading.
	UPROPERTY(EditAnywhere, Category= "Navigation System Manager")
	float UnloadTimer = 5;

	UPROPERTY()
	TArray<AActor*> NavVolumeArray;
};
