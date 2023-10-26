// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <vector>

class NavNode
{
public:
	FIntVector Coordinates;

	std::vector<NavNode*> Neighbors;

	float FScore = FLT_MAX;
	float GScore = FLT_MAX;
	float HScore = FLT_MAX;
};






