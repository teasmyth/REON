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

private:
	UPROPERTY(EditAnywhere, Category= "Navigation System Manager")
	AActor* TargetActor = nullptr;

	UPROPERTY(EditAnywhere, Category= "Navigation System Manager")
	AActor* AI_AgentActor = nullptr;
};
