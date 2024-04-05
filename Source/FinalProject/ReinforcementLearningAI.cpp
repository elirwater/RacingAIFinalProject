// Fill out your copyright notice in the Description page of Project Settings.

#include <float.h>
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



// Reference to the Splin that is set in the blueprint for this class
TArray<FVector> UReinforcementLearningAI::getModelGeneratedSplinePoints(TArray<FVector> segmentPoints) {
	// TEMPORARY IMPLEMENTATION TO TEST 
	// NORMALLY, THIS WOULD RETRIEVE THIS ITERATIONS OF THE MODELS GENERATED POINTS
	//////////////////////////////////////////////////////////////////////////////


	// Populate the array with your points...

	// Define the array to store the selected points
	TArray<FVector> ModelGeneratedPath;

	// Iterate through the array of FVector points
	for (int32 i = 0; i < segmentPoints.Num(); i += 5)
	{
		// Check if there are at least 5 points remaining
		if (i + 5 <= segmentPoints.Num())
		{
			// Select a random index from the current group of 5 points
			int32 RandomIndex = FMath::RandRange(i, i + 4);

			// Add the selected point to the ModelGeneratedPath
			ModelGeneratedPath.Add(segmentPoints[RandomIndex]);
		}
	}

	return ModelGeneratedPath;

}

float UReinforcementLearningAI::FindQValueWithMatchingStateAndAction(const FModelState& TargetState, const FPointModificationAction& TargetAction)
{
	// Iterate through the QTable array
	for (FQTableEntry& Entry : qTable)
	{
		// Find the correct state in the table
		if (Entry.State == TargetState)
		{
			// Loop through actions and find correct one
			for (FPointModificationAction action : Entry.ActionsFromState) {

				if (action == TargetAction) {
					return action.score;
				}
			}
		}
	}

	// Because we can't properly instantiate the entire Q table given the number of possibilites of states for just 10 segments rows is 5^10, if we don't have it in the table, we give it an infinite score
	// TODO: this might be a problem...
	return FLT_MAX;;
}

float UReinforcementLearningAI::GetMinQValForState(FModelState state) {

	float minVal = FLT_MAX;
	// Iterate through the QTable array
	for (FQTableEntry& Entry : qTable)
	{
		// Find the correct state in the table
		if (Entry.State == state)
		{
			// Loop through actions and modify the minVal if a smaller one is found
			for (FPointModificationAction action : Entry.ActionsFromState) {
				if (action.score < minVal) {
					minVal = action.score;
				}
			}
		}
	}
	return minVal;
}


FPointModificationAction UReinforcementLearningAI::GetMinQValAction(FModelState state) {

	float minVal = FLT_MAX;
	FPointModificationAction outAction;

	// Iterate through the QTable array
	for (FQTableEntry& Entry : qTable)
	{
		// Find the correct state in the table
		if (Entry.State == state)
		{
			// Loop through actions and modify the minVal if a smaller one is found
			for (FPointModificationAction action : Entry.ActionsFromState) {
				if (action.score < minVal) {
					minVal = action.score;
					outAction = action;
				}
			}
		}
	}

	FString LapTimeString = FString::Printf(TEXT("Finding minimum action from state..."));
	GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Orange, LapTimeString);

	return outAction;
}


void UReinforcementLearningAI::UpdateQTable(FModelState currentStateInput, FPointModificationAction selectedAction, int32 score, FModelState nextState, double alpha, double gamma) {

	// WEEEEEE NEED TO INTIALIZE Q TABLE FIRST !!!!!!! //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	  // Get the Q-value of the current state-action pair


	float oldValue = FindQValueWithMatchingStateAndAction(currentStateInput, selectedAction);

	float minValue = GetMinQValForState(nextState);

	float newVal = oldValue + alpha * (score + gamma * minValue - oldValue);

	// Iterate through the QTable array
	for (FQTableEntry& Entry : qTable)
	{
		// Find the correct state in the table
		if (Entry.State == currentStateInput)
		{
			// Loop through actions and modify the minVal if a smaller one is found
			for (FPointModificationAction action : Entry.ActionsFromState) {

				if (action == selectedAction) {

					action.score = newVal;
					// Yah, we updated our value
					return;
				}
			}
			// If we got here, that means we do have an entry for this state, but not for this select action, so we add the new action
			FPointModificationAction newAction = selectedAction;
			newAction.score = score;
			Entry.ActionsFromState.Add(newAction);

		}
	}
	// Looks like we don't have an entry yet for this currentStateInput and select action -> let's make one
	FPointModificationAction newAction = selectedAction;
	newAction.score = score;
	TArray<FPointModificationAction> actions;
	actions.Add(newAction);

	FQTableEntry entry = FQTableEntry(currentStateInput, actions);
}


FPointModificationAction UReinforcementLearningAI::GenerateRandomAction(TArray<int32> availablePointIndiciesForSegment) {

	FString LapTimeString = FString::Printf(TEXT("Generating random action..."));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, LapTimeString);

	// Calculate the number of subsections (a subsection is a group of 5 points within a given segment)
	int32 NumSubsections = availablePointIndiciesForSegment.Num() / 5;

	// Randomly select one subsection
	int32 RandomSubsectionIndex = FMath::RandRange(0, NumSubsections - 1);

	// Calculate the start index of the chosen subsection
	int32 StartIndex = RandomSubsectionIndex * 5;

	// Randomly select one point within the chosen subsection
	int32 RandomPointIndexToReplace = FMath::RandRange(StartIndex, StartIndex + 4);

	// Randomly point index to replace with new one
	int32 RandomPointIndexNew = FMath::RandRange(StartIndex, StartIndex + 4);

	FPointModificationAction action = FPointModificationAction(RandomPointIndexToReplace, RandomPointIndexNew);
	
	return action;

}




FPointModificationAction UReinforcementLearningAI::EpsilonGreedyPolicyGenerateAction(FModelState currentStateInput, double epsilon, TArray<int32> availablePointIndiciesForSegment) {

	// Generate a random number between 0 and 1
	float randomValue = FMath::FRand();

	// Check if the random value is less than epsilon (explore)
	if (randomValue < epsilon) {
		FPointModificationAction action = GenerateRandomAction(availablePointIndiciesForSegment);
		return action;
	}
	else { 
		FPointModificationAction action = GetMinQValAction(currentStateInput);

		// We don't seem to have anything in our table of value, let's generate a random point instead!
		if (!action.IsInitialized()) {

			FString LapTimeString = FString::Printf(TEXT("Failed to find minimum action, initializing random action from state..."));
			GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Orange, LapTimeString);

			action = GenerateRandomAction(availablePointIndiciesForSegment);
			return action;
		}
		return action;
	}

}