// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IActivatableObject.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UIActivatableObject : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class CHASING_5SD073_API IIActivatableObject
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	
	bool bIsActivated = false;
	bool bIsLoopingActivation = false;

	void SetActivated(const bool bNewState) { bIsActivated = bNewState; }
	bool IsActivated() const { return bIsActivated; }

	bool IsLoopingActivation() const { return bIsLoopingActivation; }

	virtual void ActivateObject() = 0;
};
