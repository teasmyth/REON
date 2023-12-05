// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */

class CHASING_5SD073_API ICharacterState
{
public:
	ICharacterState();
	virtual ~ICharacterState();

	virtual void OnEnterState();
	virtual void OnUpdateState();
	virtual void OnExitState();

	virtual const char* GetStateTypeName() const = 0;
};
