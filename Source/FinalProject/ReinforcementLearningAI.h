// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SplineController.h"
#include "ReinforcementLearningAI.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FINALPROJECT_API UReinforcementLearningAI : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UReinforcementLearningAI();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Available AINavPoints -> set by retrieveAvailableAINAvPoints()
	TArray<FVector> availableAINavPoints;

	// Called by the controller to retrieve reinforcement learning model generated points (***or this iteration/epoch of the model***)
	TArray<FVector> getModelGeneratedSplinePoints();

	// Retrieves the available AI Nav points on the racetrack from the SplineController
	void retrieveAvailableAINAvPoints();

	// Reference to the Splin that is set in the blueprint for this class
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actors")
	USplineController* SplineComponent;





		
};
