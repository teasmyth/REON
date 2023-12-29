// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class ETestCharacterState : uint8
{
	DefaultState,
	Sliding,
	WallClimbing,
	WallRunning,
	AirDashing,
	// Add other states as needed
};
