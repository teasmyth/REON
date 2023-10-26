// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NavSystemNode.h"
#include "NavSystemVolume.h"
#include "NavSystemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CHASING_5SD073_API UNavSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UNavSystemComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Finds a path from the starting location to the destination
	UFUNCTION(BlueprintCallable, Category = "Navigation System")
	bool FindPath(const FVector& start, AActor* target,
	              const TArray<TEnumAsByte<EObjectTypeQuery>>& object_types,
	              const float& meshBounds, UClass* actor_class_filter,
	              FVector& OutDirectionToMove, const bool& useAStar);


	UPROPERTY(VisibleAnywhere, Category = "Navigation System|Debug")
	ANavSystemVolume* AgentVolume = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Navigation System|Debug")
	ANavSystemVolume* PreviousAgentVolume = nullptr;

	UPROPERTY(EditAnywhere, Category = "Navigation System|Debug")
	ANavSystemVolume* TargetVolume = nullptr;

	UPROPERTY(EditAnywhere, Category = "Navigation System|Debug")
	ANavSystemVolume* PreviousTargetVolume= nullptr;

	void SetAI_AgentActorNavSystemVolume(ANavSystemVolume* NavSystemVolume) { AgentVolume = NavSystemVolume; }

	void SetTargetActorNavSystemVolume(ANavSystemVolume* NavSystemVolume) { TargetVolume = NavSystemVolume; }

	void SetTarget(AActor* TargetActor) { Target = TargetActor; }

	UFUNCTION(BlueprintPure, Category = "Navigation System")
	ANavSystemVolume* GetAgentActorNavSystemVolume() const { return AgentVolume; }

	UFUNCTION(BlueprintPure, Category = "Navigation System")
	ANavSystemVolume* GetTargetActorNavSystemVolume() const { return TargetVolume; }

	UFUNCTION(BlueprintPure, Category = "Navigation System")
	AActor* GetTargetActor() const { return Target; }

	//Decides whether to use normal pathfinding. If true, then passes out direction to move.
	UFUNCTION(BlueprintCallable, Category = "Navigation System")
	bool DecideNavigation(FVector& OutDirectionToMove);

protected:
	virtual void BeginPlay() override;

private:
	bool Jump(const ANavSystemVolume& NavVolume, const NavSystemNode& CurrentNode, const NavSystemNode& Neighbor, AActor* TargetActor,
	          NavSystemNode** OutJumpPoint);

	void AddNeighbors(const ANavSystemVolume& NavVolume, NavSystemNode* CurrentNode);

	float CalculateMaxSweepDistance(const ANavSystemVolume& NavVolume, const NavSystemNode& CurrentNode, const FVector& Direction);

	void ClampCoordinates(const ANavSystemVolume& NavVolume, FIntVector& Coordinates);

	bool AreCoordinatesValid(const ANavSystemVolume& NavVolume, const FIntVector& Coordinates);


	/**
	* Converts a coordinate into a world space location. If the coordinate is not within the bounds of the grid,
	* the coordinate will be clamped to the closest coordinate.
	* @param NavVolume Which NavSystemVolume to use
	* @param	Coordinates			The coordinates to convert into world space
	* @return	The converted location in world space
	*/
	FVector ConvertCoordinatesToLocation(const ANavSystemVolume& NavVolume, const FIntVector& Coordinates);

	/**
	* Converts a world space location to a coordinate in the grid. If the location is not located within the grid,
	* the coordinate will be clamped to the closest coordinate.
	* @param NavVolume Which NavSystemVolume to use
	* @param	Location			The location to convert
	* @return	The converted coordinates
	*/
	FIntVector ConvertLocationToCoordinates(const ANavSystemVolume& NavVolume, const FVector& Location);

	NavSystemNode* GetNode(const ANavSystemVolume& NavVolume, FIntVector Coordinates);

	NavSystemNode* StartNode = nullptr;
	NavSystemNode* LastStartNode = nullptr;

	NavSystemNode* EndNode = nullptr;
	NavSystemNode* LastEndNode = nullptr;

	UPROPERTY()
	AActor* Target = nullptr;
};

struct FHitResultCompare
{
	bool operator()(const FHitResult& left, const FHitResult& right) const
	{
		return left.Distance < right.Distance;
	}
};

struct FNodeCompare
{
	//Will put the highest FScore above all
	bool operator()(const NavSystemNode* NavSystemNode1, const NavSystemNode* NavSystemNode2) const
	{
		return (NavSystemNode1->FScore > NavSystemNode2->FScore);
	}
};
