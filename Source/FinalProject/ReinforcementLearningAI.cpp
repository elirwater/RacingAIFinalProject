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



float UReinforcementLearningAI::FindQValueWithMatchingStateAndAction(FState currentStateInput, FAction& selectedAction) {
	// THIS FUNCTION SHOULD NOT BE DOING THE SETTING HERE.......



	// Iterate through the Q Table
	for (FQTableEntry& Entry : qTable) {

		if (Entry.State == currentStateInput) {

			for (FAction& action : Entry.ActionsFromState) {

				if (action == selectedAction) {
					return action.Score;
				}
			}
			// We have an entry for the current state, but no actions for that entry, so we have to add this one
			TArray<FAction> actions = { selectedAction };
			Entry.ActionsFromState = actions;
			return -1.f;
		}
	}

	// If we make it here, that means no Q table entry exists for this state, so we have to add it
	TArray<FAction> actions = { selectedAction };
	qTable.Add(FQTableEntry(currentStateInput, actions));

	return -1.f;
}


// Function to retrieve the action with the minimum score for a given state
FAction GetActionWithMinimumScore(FQTableEntry& entry) {

	if (entry.ActionsFromState.Num() == 0) {
		// Return a default action if the array is empty
		return FAction();
	}

	float MinScore = MAX_FLT;
	FAction MinScoreAction = FAction();

	// Iterate through the actions in the array
	for (FAction& Action : entry.ActionsFromState) {
		// Check if the score of the current action is less than the minimum score, and make sure it's not a semi-initialized action
		if ((Action.Score < MinScore) && (Action.Score != -1)) {
			// Update the minimum score and corresponding action
			MinScore = Action.Score;
			MinScoreAction = Action;
		}
	}

	// Return the action with the minimum score
	return MinScoreAction;
}


void UReinforcementLearningAI::UpdateQTable(FState currentStateInput, FAction& selectedAction, float score, FState nextState, double alpha, double gamma) {

	// WEEEEEE NEED TO INTIALIZE Q TABLE FIRST !!!!!!! //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	 // Get the Q-value of the current state-action pair

	// We update our selected Actions score
	selectedAction.Score = score;

	float oldValue = FindQValueWithMatchingStateAndAction(currentStateInput, selectedAction);

	// We didn't have an entry for this state-action pair, so we created one instead
	if (oldValue == -1.f) {
		return;
	}

	float minValue = -1;
	// Find the minimum value available from this state
	for (FQTableEntry& entry : qTable) {
		if (entry.State == nextState) {
			FAction minValueAction = GetActionWithMinimumScore(entry);
			
			// NOT SURE ABOUT SOME OF THIS.........
			if (!minValueAction.bIsInitialized) {
				return;
			}

			minValue = minValueAction.Score;
		}
	}

	// Not sure about this either...
	if (minValue == -1) {
		return;
	}

	float newVal = oldValue + alpha * (score + gamma * minValue - oldValue);

	// Iterate through the QTable array to update our Q value
	for (FQTableEntry entry : qTable) {
		if (entry.State == currentStateInput) {
			for (FAction& action : entry.ActionsFromState) {
				if (action == selectedAction) {
					action.Score = newVal;
				}
			}
		}
	}


//	float oldValue = FindQValueWithMatchingStateAndAction(currentStateInput, selectedAction);
//
//	float minValue = GetMinQValForState(nextState);
//
//	float newVal = oldValue + alpha * (score + gamma * minValue - oldValue);
//
//	// Iterate through the QTable array
//	for (FQTableEntry& Entry : qTable)
//	{
//		// Find the correct state in the table
//		if (Entry.State == currentStateInput)
//		{
//			// Loop through actions and modify the minVal if a smaller one is found
//			for (FPointModificationAction action : Entry.ActionsFromState) {
//
//				if (action == selectedAction) {
//
//					action.score = newVal;
//					// Yah, we updated our value
//					return;
//				}
//			}
//			// If we got here, that means we do have an entry for this state, but not for this select action, so we add the new action
//			FPointModificationAction newAction = selectedAction;
//			newAction.score = score;
//			Entry.ActionsFromState.Add(newAction);
//
//		}
//	}
//	// Looks like we don't have an entry yet for this currentStateInput and select action -> let's make one
//	FPointModificationAction newAction = selectedAction;
//	newAction.score = score;
//	TArray<FPointModificationAction> actions;
//	actions.Add(newAction);
//
//	FQTableEntry entry = FQTableEntry(currentStateInput, actions);
//}
}



TArray<FAction> UReinforcementLearningAI::GeneratePossibleActionFromState(FState currentState) {

	TArray<FAction> outputActions;

	// We iterate through the available segments
	for (FRacingLineForSegment SegmentRacingLine : currentState.SegmentRacingLines)
	{
		// Go through 5 possible racing lines (even though there are only 4 available ones to choose from)
		for (int32 i = 0; i < 5; i += 1)
		{
			// We check to make sure we aren't going to add the racing line already in use
			if (i != SegmentRacingLine.RacingLineIndex) {
				// Remeber, the newly initialized Faction has a score of -1 (which means we havne't set a score yet)
				FAction newAction = FAction(SegmentRacingLine.SegmentIndex, i);
				outputActions.Add(newAction);
			}
		}
	}
	return outputActions;
}


FAction UReinforcementLearningAI::GenerateRandomAction(FState currentState) {

	FString stringToUse = FString::Printf(TEXT("Generating random action..."));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, stringToUse);

	// Gather our possible actions from this state
	TArray<FAction> possibleActions = GeneratePossibleActionFromState(currentState);

	// Randomly pick an index
	int32 RandomSubsectionIndex = FMath::RandRange(0, possibleActions.Num() - 1);

	return possibleActions[RandomSubsectionIndex];
}




FAction UReinforcementLearningAI::EpsilonGreedyPolicyGenerateAction(FState currentStateInput, double epsilon) {

	// Generate a random number between 0 and 1
	float randomValue = FMath::FRand();

	// Check if the random value is less than epsilon (explore)
	if (randomValue < epsilon) {
		FAction action = GenerateRandomAction(currentStateInput);
		return action;
	} 
	else {

		FString stringToUse = FString::Printf(TEXT("Finding previous action..."));
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, stringToUse);

		FAction MinScoreAction = FAction();

		// Loop through our Q table to find the current state we are in
		for (FQTableEntry entry : qTable) {
			if (entry.State == currentStateInput) {
				MinScoreAction = GetActionWithMinimumScore(entry);
			}
		}

		// If we couldn't find anything...
		if (!MinScoreAction.bIsInitialized) {
			FAction action = GenerateRandomAction(currentStateInput);
			return action;
		}

		return MinScoreAction;
	}
}