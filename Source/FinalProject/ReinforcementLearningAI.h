// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SplineController.h"
#include <float.h>
#include "ReinforcementLearningAI.generated.h"




// Custom struct to represent which racing line was choosen for a given segment
USTRUCT(BlueprintType)
struct FRacingLineForSegment {
	GENERATED_BODY()


	// Which segment is this racing line applied to 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SegmentIndex;

	// Which racing is choosen for this segment
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RacingLineIndex;

	bool operator==(const FRacingLineForSegment& Other) const
	{
		return (RacingLineIndex == Other.RacingLineIndex) && (SegmentIndex == Other.SegmentIndex);
	}
};

// Custom struct to represent an action that can be taken by the RL algorithm
USTRUCT(BlueprintType)
struct FAction {
	GENERATED_BODY()

	// Which segment is this racing line applied to 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SegmentToModify;

	// Which new racing is choosen for this segment
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NewRacingLineIndex;

	// The score this action generated
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Score;

	// Has this action been initialized yet?
	bool bIsInitialized;

	FAction() : bIsInitialized(false) {}

	FAction(int32 InputSegmentToModify, int32 InputNewRacingLineIndex) : SegmentToModify(InputSegmentToModify), NewRacingLineIndex(InputNewRacingLineIndex), bIsInitialized(true), Score(-1.f) {}

	bool operator==(const FAction& Other) const
	{
		return (SegmentToModify == Other.SegmentToModify) && (NewRacingLineIndex == Other.NewRacingLineIndex);
	}
};


// Custom struct to represent a State for the RL algorithm
USTRUCT(BlueprintType)
struct FState {
	GENERATED_BODY()

	// A given state is a description of each segment and their racing lines
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FRacingLineForSegment> SegmentRacingLines;

	// Has this state been initialized yet?
	bool bIsInitialized;

	FState() : bIsInitialized(false) {}

	FState(TArray<FRacingLineForSegment> InputSegmentRacingLines) : SegmentRacingLines(InputSegmentRacingLines), bIsInitialized(true) {}

	bool operator==(const FState& Other) const
	{
		for (int i = 0; i < SegmentRacingLines.Num(); i += 1) {
			if (SegmentRacingLines[i] != Other.SegmentRacingLines[i]) {
				return false;
			}
		}
		return true;
	}
};


// Custom struct to represent a Q Table entry
USTRUCT(BlueprintType)
struct FQTableEntry {
	GENERATED_BODY()

	FState State;

	TArray<FAction> ActionsFromState;

	FQTableEntry() {}

	FQTableEntry(FState InputState, TArray<FAction> InputActionFromState) : State(InputState), ActionsFromState(InputActionFromState) {}

};


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


	// Reference to the Splin that is set in the blueprint for this class
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actors")
	USplineController* SplineComponent;


	// Where the Q Table is stored
	TArray<FQTableEntry> qTable;

	// Generates a list of possible actions from this current state
	TArray<FAction> GeneratePossibleActionFromState(FState currentState);

	// Generates a random action given the current state
	FAction GenerateRandomAction(FState currentState);

	// Runs the greedy action generation (called by simulation controlller)
	FAction EpsilonGreedyPolicyGenerateAction(FState currentState, double epsilon);

	// Updates the Q table for a given action-state pair with the lap time score
	void UpdateQTable(FState currentState, FAction& selectedAction, float score, FState nextState, double alpha, double gamma);

	// Finds the best Q value from a current state-action pair
	float FindQValueWithMatchingStateAndAction(FState currentStateInput, FAction& selectedAction);

	// Retrieves the best state that is in the Q table
	FState GetBestState();



};
