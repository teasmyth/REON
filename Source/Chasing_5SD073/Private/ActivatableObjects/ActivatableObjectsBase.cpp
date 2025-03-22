// Fill out your copyright notice in the Description page of Project Settings.


#include "ActivatableObjects/ActivatableObjectsBase.h"

// Sets default values for this component's properties
UActivatableObjectsBase::UActivatableObjectsBase()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatFinder(
		TEXT("/Game/Materials/PostProcessing/MI_ObjectOutlineShader_PP_Inst.MI_ObjectOutlineShader_PP_Inst"));


	if (MatFinder.Succeeded())
	{
		HighlightMaterial = MatFinder.Object;
		
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Highlight Material not found!"));
	}

	// ...
}


// Called when the game starts
void UActivatableObjectsBase::BeginPlay()
{
	Super::BeginPlay();
	Mesh = GetOwner()->GetComponentByClass<UStaticMeshComponent>();
	

	// ...
}


// Called every frame
void UActivatableObjectsBase::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CanExecuteAction()) return;

	if (bIsSwitchAction)
	{
		if (bIsActivated)
		{
			DoAction(DeltaTime);
		}
		else
		{
			UndoAction(DeltaTime);
		}
	}
	else
	{
		if (bIsActivated)
		{
			DoAction(DeltaTime);
		}
	}

	// ...
}

void UActivatableObjectsBase::ActivateObject()
{
	bIsActivated = !bIsActivated;
}

void UActivatableObjectsBase::SetHighlight(const bool bEnable) const
{
	if (Mesh)
	{
		if (!HighlightMaterial)
		{
			UE_LOG(LogTemp, Warning, TEXT("Highlight Material not found!"));
			return;
		}

		Mesh->SetOverlayMaterial(bEnable ? HighlightMaterial : nullptr);
	}
}

void UActivatableObjectsBase::DoAction(const float DeltaTime) const 
{
	//Override this function in the child class
	UE_LOG(LogTemp, Warning, TEXT("Doing Action: %s"), *GetOwner()->GetName());
}

void UActivatableObjectsBase::UndoAction(const float DeltaTime) const 
{
	//Override this function in the child class
	UE_LOG(LogTemp, Warning, TEXT("Undoing Action: %s"), *GetOwner()->GetName());
}

bool UActivatableObjectsBase::CanExecuteAction() const
{
	//Override this function in the child class
	return true;
}
