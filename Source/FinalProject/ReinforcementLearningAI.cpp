// Fill out your copyright notice in the Description page of Project Settings.


#include "ReinforcementLearningAI.h"

// Sets default values for this component's properties
UReinforcementLearningAI::UReinforcementLearningAI()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UReinforcementLearningAI::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UReinforcementLearningAI::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

// Retrieves the available AI Nav points on the racetrack from the SplineController
void UReinforcementLearningAI::retrieveAvailableAINAvPoints() {

	if (SplineComponent) {
		availableAINavPoints = SplineComponent->getAvailableAINavPoints();
	}
}


// Reference to the Splin that is set in the blueprint for this class
TArray<FVector> UReinforcementLearningAI::getModelGeneratedSplinePoints() {
	// TEMPORARY IMPLEMENTATION TO TEST 
	// NORMALLY, THIS WOULD RETRIEVE THIS ITERATIONS OF THE MODELS GENERATED POINTS
	//////////////////////////////////////////////////////////////////////////////


	// Randomly generate a list of points by picking randomly 1 point in every row 
	retrieveAvailableAINAvPoints();

	// Populate the array with your points...

	// Define the array to store the selected points
	TArray<FVector> ModelGeneratedPath;

	// Iterate through the array of FVector points
	for (int32 i = 0; i < availableAINavPoints.Num(); i += 5)
	{
		// Check if there are at least 5 points remaining
		if (i + 5 <= availableAINavPoints.Num())
		{
			// Select a random index from the current group of 5 points
			int32 RandomIndex = FMath::RandRange(i, i + 4);

			// Add the selected point to the ModelGeneratedPath
			ModelGeneratedPath.Add(availableAINavPoints[RandomIndex]);
		}
	}

	return ModelGeneratedPath;

}

