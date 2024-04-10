// Fill out your copyright notice in the Description page of Project Settings.

#include <float.h>
#include "ReinforcementLearningAI.h"

// Sets default values for this component's properties
UReinforcementLearningAI::UReinforcementLearningAI()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}


void UReinforcementLearningAI::BeginPlay()
{
	Super::BeginPlay();

	
}


void UReinforcementLearningAI::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}



float UReinforcementLearningAI::FindQValueWithMatchingStateAndAction(FState currentStateInput, FAction& selectedAction) {

	// Iterate through the Q Table
	for (FQTableEntry& Entry : qTable) {

		if (Entry.State == currentStateInput) {

			for (FAction& action : Entry.ActionsFromState) {

				if (action == selectedAction) {
					return action.Score;
				}
			}
			// We have an entry for the current state, but no actions for that entry, so we have to add this one
			//TArray<FAction> actions = { selectedAction };
			Entry.ActionsFromState.Add(selectedAction);
			return -1.f;
		}
	}

	// If we make it here, that means no Q table entry exists for this state, so we have to add it
	TArray<FAction> actions = { selectedAction };
	qTable.Add(FQTableEntry(currentStateInput, actions));

	// Negative 1 means unintialized 
	return -1.f;
}


// Function to retrieve the action with the minimum score from a given state
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

	// If the actions are all MAX_FLT scores, we still want to pass that value back to be used, so we set the score of our default action as such
	if (MinScore == MAX_FLT) {
		MinScoreAction.bIsInitialized = true;
		MinScoreAction.Score = MAX_FLT;
	}

	// Return the action with the minimum score
	return MinScoreAction;
}


void UReinforcementLearningAI::UpdateQTable(FState currentStateInput, FAction& selectedAction, float score, FState nextState, double alpha, double gamma) {

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
			
			if (!minValueAction.bIsInitialized) {
				return;
			}

			minValue = minValueAction.Score;
		}
	}

	if (minValue == -1) {
		return;
	}

	// Our updated Q-learning value for this state-action pair
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
}


FState UReinforcementLearningAI::GetBestState() {

	// Find the minimum value available from this state
	float MinScore = MAX_FLT;
	FState MinState;

	// Loop through every entry in the Q table, and find the best action score, and then update the state based on that score and return it
	for (FQTableEntry& entry : qTable) {
		for (FAction action : entry.ActionsFromState) {
			if (action.bIsInitialized) {
				if (action.Score < MinScore) {
					MinScore = action.Score;
					MinState = entry.State;

					for (FRacingLineForSegment& rLine : MinState.SegmentRacingLines) {
						if (rLine.SegmentIndex == action.SegmentToModify) {
							rLine.RacingLineIndex = action.NewRacingLineIndex;
						}
					}
				}
			}
		}
	}
	return MinState;
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
				// Remember, the newly initialized Faction has a score of -1 (which means we havne't set a score yet)
				FAction newAction = FAction(SegmentRacingLine.SegmentIndex, i);
				outputActions.Add(newAction);
			}
		}
	}
	return outputActions;
}


FAction UReinforcementLearningAI::GenerateRandomAction(FState currentState) {

	FString stringToUse = FString::Printf(TEXT("Generating random action..."));
	GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Orange, stringToUse);

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

		FString stringToUse = FString::Printf(TEXT("Finding previous action from this state..."));
		GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Orange, stringToUse);

		FAction MinScoreAction = FAction();

		// Loop through our Q table to find the current state we are in
		for (FQTableEntry entry : qTable) {
			if (entry.State == currentStateInput) {
				MinScoreAction = GetActionWithMinimumScore(entry);
			}
		}

		// If we couldn't find anything action OR the action was set to float max because it hit something so we would want to generate a new random one anyway...
		if (!MinScoreAction.bIsInitialized || MinScoreAction.Score == MAX_FLT) {
			FAction action = GenerateRandomAction(currentStateInput);
			return action;
		}

		return MinScoreAction;
	}
}