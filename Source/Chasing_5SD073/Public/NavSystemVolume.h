// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
//#include "NavSystemComponent.h"
#include "GameFramework/Actor.h"
#include "NavSystemNode.h"
#include "NavSystemVolume.generated.h"

class UProceduralMeshComponent;
class NavSystemNode;

UCLASS()
class CHASING_5SD073_API ANavSystemVolume : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANavSystemVolume();

private:
	// The default (root) scene component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation System Volume", meta = (AllowPrivateAccess = "true"))
	USceneComponent* DefaultSceneComponent = nullptr;

	// The procedural mesh responsbile for rendering the grid
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation System Volume", meta = (AllowPrivateAccess = "true"))
	UProceduralMeshComponent* ProceduralMesh = nullptr;

	// The number of divisions in the grid along the X axis
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation System Volume|Grid Settings",
		meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 DivisionsX = 10;

	// The number of divisions in the grid along the Y axis
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation System Volume|Grid Settings",
		meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 DivisionsY = 10;

	// The number of divisions in the grid along the Z axis
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation System Volume|Grid Settings",
		meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 DivisionsZ = 10;

	// The size of each division
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation System Volume|Grid Settings",
		meta = (AllowPrivateAccess = "true", ClampMin = 1))
	float DivisionSize = 100.0f;

	// The minimum number of axes that must be shared with a neighboring node for it to be counted a neighbor
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation System Volume|Grid Settings",
		meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 2))
	int32 MinSharedNeighborAxes = 0;

	// The thickness of the grid lines
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation System Volume|Aesthetics", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	float LineThickness = 2.0f;

	// The color of the grid
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation System Volume|Aesthetics", meta = (AllowPrivateAccess = "true"))
	FLinearColor Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.5f);

	//Instead of of just borders, draws a whole 3D object.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation System Volume|Aesthetics", meta = (AllowPrivateAccess = "true"))
	bool DrawParallelogram = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation System Volume|Debug", meta = (AllowPrivateAccess = "true"))
	FVector TargetActorEnterLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation System Volume|Debug", meta = (AllowPrivateAccess = "true"))
	FVector TargetActorEndLocation;

public:
	/**
	* Called when an instance of this class is placed (in editor) or spawned.
	* @param	Transform			The transform the actor was constructed at.
	*/
	virtual void OnConstruction(const FTransform& Transform) override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	//Visualizes the grid.
	UFUNCTION(CallInEditor, Category = "Navigation System Volume")
	void CreateGrid();

	//Makes borders.
	UFUNCTION(CallInEditor, Category = "Navigation System Volume")
	void CreateBorders();

	//Deletes the visualized grid.
	UFUNCTION(CallInEditor, Category = "Navigation System Volume")
	void DeleteGrid() const;


	// Gets the total number of divisions in the grid
	UFUNCTION(BlueprintPure, Category = "Navigation System Volume")
	FORCEINLINE int32 GetTotalDivisions() const { return DivisionsX * DivisionsY * DivisionsZ; }

	// Gets the number of x divisions in the grid
	UFUNCTION(BlueprintPure, Category = "Navigation System Volume")
	FORCEINLINE int32 GetDivisionsX() const { return DivisionsX; }

	// Gets the number of y divisions in the grid
	UFUNCTION(BlueprintPure, Category = "Navigation System Volume")
	FORCEINLINE int32 GetDivisionsY() const { return DivisionsY; }

	// Gets the number of z divisions in the grid
	UFUNCTION(BlueprintPure, Category = "Navigation System Volume")
	FORCEINLINE int32 GetDivisionsZ() const { return DivisionsZ; }

	// Gets the size of each division in the grid
	UFUNCTION(BlueprintPure, Category = "Navigation System Volume")
	FORCEINLINE float GetDivisionSize() const { return DivisionSize; }

	// Gets the size of the grid along the X axis
	float GetGridSizeX() const { return DivisionsX * DivisionSize; }

	// Gets the size of the grid along the Y axis
	float GetGridSizeY() const { return DivisionsY * DivisionSize; }

	// Gets the size of the grid along the Z axis
	float GetGridSizeZ() const { return DivisionsZ * DivisionSize; }

	NavSystemNode* GetNodeArray() const { return Nodes; }

	int32 GetMinSharedNeighborAxes() const { return MinSharedNeighborAxes; }

	bool GetAreNodesLoaded() const { return bAreNodesLoaded; }

	void SetTargetActor(AActor* Actor) { TargetActor = Actor; }

	void SetAI_AgentActor(AActor* Actor) { AI_AgentActor = Actor; }

	void SetUnloadTimer(const float Timer) { UnloadTimer = Timer;}

	FVector GetTargetActorEnterLocation() const { return TargetActorEnterLocation; }
	FVector GetTargetActorEndLocation() const { return TargetActorEndLocation; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Overridable function called whenever this actor is being removed from a level
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void PopulateNodesAsync();

	// Helper function for creating the geometry for a single line of the grid
	void CreateLine(const FVector& start, const FVector& end, const FVector& normal, TArray<FVector>& vertices, TArray<int32>& triangles) const;

	/*The front face's corners (usually clockwise or counterclockwise):
	Top-left-front corner
	Top-right-front corner
	Bottom-right-front corner
	Bottom-left-front corner

	The back face's corners (matching order with the front face):
	Top-left-back corner
	Top-right-back corner
	Bottom-right-back corner
	Bottom-left-back corner
	*/
	void CreateCubeMesh(const FVector& Corner1, const FVector& Corner2, const FVector& Corner3, const FVector& Corner4, const FVector& Corner5,
	                    const FVector& Corner6, const FVector& Corner7, const FVector& Corner8);


	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                  int32 OtherBodyIndex);

	void UnloadGridAsync();

	//Visible for debug purposes.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation System Volume|Debug", meta = (AllowPrivateAccess = "true"))
	AActor* TargetActor = nullptr;

	//Visible for debug purposes.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation System Volume|Debug", meta = (AllowPrivateAccess = "true"))
	AActor* AI_AgentActor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation System Volume|Debug", meta = (AllowPrivateAccess = "true"))
	bool bAreNodesLoaded = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation System Volume|Debug", meta = (AllowPrivateAccess = "true"))
	float UnloadTimer;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation System Volume|Debug", meta = (AllowPrivateAccess = "true"))
	bool bStartUnloading = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation System Volume|Debug", meta = (AllowPrivateAccess = "true"))
	float m_unloadTimer;
	// The nodes used for Grid Settings
	NavSystemNode* Nodes = nullptr;
};
