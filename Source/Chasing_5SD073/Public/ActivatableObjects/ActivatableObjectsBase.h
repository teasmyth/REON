// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActivatableObjectsBase.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CHASING_5SD073_API UActivatableObjectsBase : public UActorComponent // also implements the interface
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UActivatableObjectsBase();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, Category = "Activatable Objects", meta = (ToolTip = "Switch type objects will only activate once. If reactivated, it will deactivate/undo the action."))
	bool bIsSwitchAction = false;
	
	UFUNCTION(BlueprintCallable, Category = "Activatable Objects")
	virtual void DoAction(const float DeltaTime) const; 

	UFUNCTION(BlueprintCallable, Category = "Activatable Objects")
	virtual void UndoAction(const float DeltaTime) const; 

	UFUNCTION(BlueprintCallable, Category = "Activatable Objects")
	virtual bool CanExecuteAction() const;

	

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	UFUNCTION(BlueprintCallable, Category = "Activatable Objects")
	virtual void ActivateObject() final;

	UFUNCTION(BlueprintCallable, Category = "Activatable Objects")
	void SetHighlight(bool bEnable) const;

private:
	bool bIsActivated = false;
	bool bIsHighlighted = false;
	
	UPROPERTY()
	UMaterialInterface* HighlightMaterial = nullptr;

	UPROPERTY()
	UStaticMeshComponent* Mesh = nullptr;
};

