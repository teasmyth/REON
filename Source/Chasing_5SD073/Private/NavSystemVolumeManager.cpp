// Fill out your copyright notice in the Description page of Project Settings.


#include "NavSystemVolumeManager.h"
#include "NavSystemComponent.h"
#include "NavSystemVolume.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ANavSystemVolumeManager::ANavSystemVolumeManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ANavSystemVolumeManager::BeginPlay()
{
	Super::BeginPlay();
	
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANavSystemVolume::StaticClass(), NavVolumeArray);
	SetUpNavVolumes();
	
}

// Called every frame
void ANavSystemVolumeManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ANavSystemVolumeManager::SetUpNavVolumes()
{
	if (NavVolumeArray.Num() > 0)
	{
		for (const auto Actor : NavVolumeArray)
		{
			ANavSystemVolume* Volume = Cast<ANavSystemVolume>(Actor);
			if (TargetActor != nullptr) Volume->SetTargetActor(TargetActor);
			if (AI_AgentActor != nullptr) Volume->SetAI_AgentActor(AI_AgentActor->GetOwner());
			Volume->SetUnloadTimer(UnloadTimer);
			Volume->SetVolumeManager(this);
		}
	}

	if (TargetActor != nullptr && AI_AgentActor != nullptr)
	{
		AI_AgentActor->SetTarget(TargetActor);
	}
}

void ANavSystemVolumeManager::SetUpTarget()
{
	if (NavVolumeArray.Num() > 0 && TargetActor != nullptr)
	{
		for (const auto Actor : NavVolumeArray)
		{
			Cast<ANavSystemVolume>(Actor)->SetTargetActor(TargetActor);
		}
	}
}

void ANavSystemVolumeManager::SetUpTarget(AActor* NewTarget)
{
	TargetActor = NewTarget;
	if (NavVolumeArray.Num() > 0)
	{
		for (const auto Actor : NavVolumeArray)
		{
			Cast<ANavSystemVolume>(Actor)->SetTargetActor(TargetActor);
		}
	}
}

void ANavSystemVolumeManager::SetUpAI_Agent()
{
	if (NavVolumeArray.Num() > 0 && AI_AgentActor != nullptr)
	{
		for (const auto Actor : NavVolumeArray)
		{
			Cast<ANavSystemVolume>(Actor)->SetAI_AgentActor(AI_AgentActor->GetOwner());
		}
	}
}

void ANavSystemVolumeManager::SetUpAI_Agent(UNavSystemComponent* NewAgent)
{
	AI_AgentActor = NewAgent;
	AI_AgentActor->SetVolumeManager(this);
	AI_AgentActor->SetTarget(TargetActor);
	//AI_AgentActor->SetAI_AgentActorNavSystemVolume(AI_AgentVolume);
	if (TargetActor != nullptr) AI_AgentActor->SetTargetActorNavSystemVolume(TargetVolume); //Only set this if Target is in game, otherwise could be outdated.
	if (NavVolumeArray.Num() > 0)
	{
		for (const auto Actor : NavVolumeArray)
		{
			Cast<ANavSystemVolume>(Actor)->SetAI_AgentActor(AI_AgentActor->GetOwner());
		}
	}
}

void ANavSystemVolumeManager::SetTargetActorVolume(ANavSystemVolume* Volume)
{
	TargetVolume = Volume;
	if (AI_AgentActor != nullptr)
	{
		AI_AgentActor->SetTargetActorNavSystemVolume(Volume);
	}
}

void ANavSystemVolumeManager::SetAI_AgentActorVolume(ANavSystemVolume* Volume)
{
	AI_AgentVolume = Volume;
	if (AI_AgentActor != nullptr)
	{
		AI_AgentActor->SetAI_AgentActorNavSystemVolume(Volume);
	}
}
