// Fill out your copyright notice in the Description page of Project Settings.


#include "NavSystemVolumeManager.h"

#include "NavSystemComponent.h"
#include "NavSystemVolume.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ANavSystemVolumeManager::ANavSystemVolumeManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ANavSystemVolumeManager::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANavSystemVolume::StaticClass(), FoundActors);

	for (const auto Actor : FoundActors)
	{
		ANavSystemVolume* Volume = Cast<ANavSystemVolume>(Actor);
		Volume->SetTargetActor(TargetActor);
		Volume->SetAI_AgentActor(AI_AgentActor);
	}

	AI_AgentActor->GetComponentByClass<UNavSystemComponent>()->SetTarget(TargetActor);

	//UE_LOG(LogTemp, Display, TEXT("Found: %i"), FoundActors.Num());
}

// Called every frame
void ANavSystemVolumeManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

