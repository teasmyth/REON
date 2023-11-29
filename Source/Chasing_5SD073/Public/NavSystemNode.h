// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <vector>

/**
 * 
 */
class CHASING_5SD073_API NavSystemNode
{
public:
	FIntVector Coordinates;

	std::vector<NavSystemNode*> Neighbors;

	float FScore = FLT_MAX;
	float GScore = FLT_MAX;
	//float HScore = FLT_MAX;
};
