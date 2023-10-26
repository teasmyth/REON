// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NavNode.h"
#include "GameFramework/Actor.h"
#include "NavigationVolume3D.generated.h"

class UProceduralMeshComponent;
class NavNode;

UCLASS()
class NAVIGATION3D_API ANavigationVolume3D : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANavigationVolume3D();

private:
	// The default (root) scene component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "A NavigationVolume3D", meta = (AllowPrivateAccess = "true"))
	USceneComponent* DefaultSceneComponent = nullptr;

	// The procedural mesh responsbile for rendering the grid
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "A NavigationVolume3D", meta = (AllowPrivateAccess = "true"))
	UProceduralMeshComponent* ProceduralMesh = nullptr;

	// The number of divisions in the grid along the X axis
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "A NavigationVolume3D|Pathfinding", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 DivisionsX = 10;

	// The number of divisions in the grid along the Y axis
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "A NavigationVolume3D|Pathfinding", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 DivisionsY = 10;

	// The number of divisions in the grid along the Z axis
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "A NavigationVolume3D|Pathfinding", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 DivisionsZ = 10;

	// The size of each division
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "A NavigationVolume3D|Pathfinding", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	float DivisionSize = 100.0f;

	// The minimum number of axes that must be shared with a neighboring node for it to be counted a neighbor
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "A NavigationVolume3D|Pathfinding",
		meta = (AllowPrivateAccess = "true", ClampMin = 0, ClampMax = 2))
	int32 MinSharedNeighborAxes = 0;

	// The thickness of the grid lines
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "A NavigationVolume3D|Aesthetics", meta = (AllowPrivateAccess = "true", ClampMin = 0))
	float LineThickness = 2.0f;

	// The color of the grid
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "A NavigationVolume3D|Aesthetics", meta = (AllowPrivateAccess = "true"))
	FLinearColor Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.5f);

	//Instead of of just borders, draws a whole 3D object.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "A NavigationVolume3D|Aesthetics", meta = (AllowPrivateAccess = "true"))
	bool DrawParallelogram = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "A NavigationVolume3D", meta = (AllowPrivateAccess = "true"))
	AActor* TargetActor = nullptr;

public:
	/**
	* Called when an instance of this class is placed (in editor) or spawned.
	* @param	Transform			The transform the actor was constructed at.
	*/
	virtual void OnConstruction(const FTransform& Transform) override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Gets the node at the specified coordinates
	NavNode* GetNode(FIntVector coordinates) const;

	//Visualizes the grid.
	UFUNCTION(CallInEditor, Category = "A NavigationVolume3D")
	void CreateGrid();

	UFUNCTION(CallInEditor, Category = "A NavigationVolume3D")
	void CreateBorders();

	//Deletes the visualized grid.
	UFUNCTION(CallInEditor, Category = "A NavigationVolume3D")
	void DeleteGrid() const;


	// Finds a path from the starting location to the destination
	UFUNCTION(BlueprintCallable, Category = "A NavigationVolume3D")
	bool FindPath(const FVector& start, const FVector& destination, AActor* target,
	              const TArray<TEnumAsByte<EObjectTypeQuery>>& object_types,
	              const float& meshBounds, UClass* actor_class_filter,
	              TArray<FVector>& out_path, const bool& useAStar);
	/**
	* Converts a world space location to a coordinate in the grid. If the location is not located within the grid,
	* the coordinate will be clamped to the closest coordinate.
	* @param	location			The location to convert
	* @return	The converted coordinates
	*/
	UFUNCTION(BlueprintCallable, Category = "A NavigationVolume3D")
	UPARAM(DisplayName = "Coordinates") FIntVector ConvertLocationToCoordinates(const FVector& location);

	/**
	* Converts a coordinate into a world space location. If the coordinate is not within the bounds of the grid,
	* the coordinate will be clamped to the closest coordinate.
	* @param	coordinates			The coordinates to convert into world space
	* @return	The converted location in world space
	*/
	UFUNCTION(BlueprintCallable, Category = "A NavigationVolume3D")
	UPARAM(DisplayName = "World Location") FVector ConvertCoordinatesToLocation(const FIntVector& coordinates);

	// Gets the total number of divisions in the grid
	UFUNCTION(BlueprintPure, Category = "A NavigationVolume3D")
	FORCEINLINE int32 GetTotalDivisions() const { return DivisionsX * DivisionsY * DivisionsZ; }

	// Gets the number of x divisions in the grid
	UFUNCTION(BlueprintPure, Category = "A NavigationVolume3D")
	FORCEINLINE int32 GetDivisionsX() const { return DivisionsX; }

	// Gets the number of y divisions in the grid
	UFUNCTION(BlueprintPure, Category = "A NavigationVolume3D")
	FORCEINLINE int32 GetDivisionsY() const { return DivisionsY; }

	// Gets the number of z divisions in the grid
	UFUNCTION(BlueprintPure, Category = "A NavigationVolume3D")
	FORCEINLINE int32 GetDivisionsZ() const { return DivisionsZ; }

	// Gets the size of each division in the grid
	UFUNCTION(BlueprintPure, Category = "A NavigationVolume3D")
	FORCEINLINE float GetDivisionSize() const { return DivisionSize; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Overridable function called whenever this actor is being removed from a level
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Gets the size of the grid along the X axis
	float GetGridSizeX() const { return DivisionsX * DivisionSize; }

	// Gets the size of the grid along the Y axis
	float GetGridSizeY() const { return DivisionsY * DivisionSize; }

	// Gets the size of the grid along the Z axis
	float GetGridSizeZ() const { return DivisionsZ * DivisionSize; }

private:
	void PopulateNodesAsync();

	// Helper function for creating the geometry for a single line of the grid
	void CreateLine(const FVector& start, const FVector& end, const FVector& normal, TArray<FVector>& vertices, TArray<int32>& triangles);


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

	// Helper function to check if a coordinate is valid
	bool AreCoordinatesValid(const FIntVector& coordinates) const;

	// Helper function to clamp the coordinate to a valid one inside the grid
	void ClampCoordinates(FIntVector& coordinates) const;

	float CalculateMaxSweepDistance(const NavNode& CurrentNode, const FVector& Direction);

	bool Jump(const NavNode& CurrentNode, const NavNode& Neighbor, AActor* Target, NavNode** outJumpPoint);

	void AddNeighbors(NavNode* currentNode) const;

	UFUNCTION()
	void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp,
	                    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	bool areNodesLoaded = false;

	// The nodes used for pathfinding
	NavNode* Nodes = nullptr;
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
	bool operator()(const NavNode* NavNode1, const NavNode* NavNode2) const
	{
		return (NavNode1->FScore > NavNode2->FScore);
	}
};
