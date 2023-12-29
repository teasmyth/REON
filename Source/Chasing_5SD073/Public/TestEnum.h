// CharacterStateEnum.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TestEnum.generated.h"

UENUM(BlueprintType)
enum class ETestEnum : uint8
{
	DefaultState,
	Sliding,
	WallClimbing,
	WallRunning,
	AirDashing,
	// Add other states as needed
};