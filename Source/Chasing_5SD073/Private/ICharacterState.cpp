// Fill out your copyright notice in the Description page of Project Settings.


#include "ICharacterState.h"

ICharacterState::ICharacterState()
{
}

ICharacterState::~ICharacterState()
{
}

void ICharacterState::OnEnterState()
{
	UE_LOG(LogTemp, Error, TEXT("Missing On Enter State Implementation."));
}

void ICharacterState::OnUpdateState()
{
	UE_LOG(LogTemp, Error, TEXT("Missing On Update State Implementation."));
}

void ICharacterState::OnExitState()
{
	UE_LOG(LogTemp, Error, TEXT("Missing On Exit State Implementation."));
}
