// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SplineController.h"
#include "ReinforcementLearningAI.generated.h"



	// Custom struct to represent an action (which is modifying 1 point in the segment
USTRUCT(BlueprintType)
struct FPointModificationAction
{
	GENERATED_BODY()
	// Index of the previous point
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PreviousPointIndex;

	// Index of the new point
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NewPointIndex;

	// Score for this action 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 score;

	bool bInitialized;

	// Default constructor
	FPointModificationAction() : PreviousPointIndex(0), NewPointIndex(0), bInitialized(false) {}

	// Constructor with parameters
	FPointModificationAction(int32 PreviousIndex, int32 NewIndex)
		: PreviousPointIndex(PreviousIndex), NewPointIndex(NewIndex), score(10000), bInitialized(true) {}

	// Equality operator
	bool operator==(const FPointModificationAction& Other) const
	{
		return (PreviousPointIndex == Other.PreviousPointIndex) && (NewPointIndex == Other.NewPointIndex);
	}

	// Function to check if the state has been initialized
	bool IsInitialized() const
	{
		return bInitialized;
	}
};


USTRUCT(BlueprintType)
// Custom struct to represent a state, 1 state includes the indexes of the x number of points used for this segment of the spline
struct FModelState
{
	GENERATED_BODY()

	// List of point indexes for this segment
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> PointIndexes;

	bool bInitialized;

	// Default constructor
	FModelState()
		: bInitialized(false)
	{
	}

	// Constructor with initialization
	FModelState(const TArray<int32>& InPointIndexes)
		: PointIndexes(InPointIndexes), bInitialized(true)
	{
	}

	// Function to set the point indexes
	void SetPointIndexes(const TArray<int32>& InPointIndexes)
	{
		PointIndexes = InPointIndexes;
		bInitialized = true;
	}

	// Equality operator
	bool operator==(const FModelState& Other) const
	{
		return PointIndexes == Other.PointIndexes;
	}

	// Inequality operator
	bool operator!=(const FModelState& Other) const
	{
		return !(*this == Other);
	}

	// Function to check if the state has been initialized
	bool IsInitialized() const
	{
		return bInitialized;
	}
};

USTRUCT(BlueprintType)
// Custom struct to represent a Q-table entry, each entry includes the state and the associated score of the state
struct FQTableEntry
{
	GENERATED_BODY()
	// State represented by the Q-table entry
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FModelState State;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPointModificationAction> ActionsFromState;

	// Default constructor
	FQTableEntry() {}

	// Constructor with parameters
	FQTableEntry(const FModelState& InState, TArray<FPointModificationAction> InActionsFromState)
		: State(InState), ActionsFromState(InActionsFromState) {}
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



	float FindQValueWithMatchingStateAndAction(const FModelState& TargetState, const FPointModificationAction& TargetAction);

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Available AINavPoints -> set by retrieveAvailableAINAvPoints()
	TArray<FVector> availableAINavPoints;

	// Called by the controller to retrieve reinforcement learning model generated points (***or this iteration/epoch of the model***)
	TArray<FVector> getModelGeneratedSplinePoints(TArray<FVector> currentSegmentPoints);


	// Reference to the Splin that is set in the blueprint for this class
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actors")
	USplineController* SplineComponent;



	TArray<FQTableEntry> qTable;
	
	void UpdateQTable(FModelState currentState, FPointModificationAction selectedAction, int32 score, FModelState nextState, double alpha, double gamma);

	FPointModificationAction EpsilonGreedyPolicyGenerateAction(FModelState currentState, double epsilon, TArray<int32> availablePointIndiciesForSegment);
	
	FModelState currentState;

	float GetMinQValForState(FModelState state);


	TArray<FPointModificationAction> possibleActionsForThisSegment;

	FPointModificationAction GenerateRandomAction(TArray<int32> availablePointIndiciesForSegment);

	FPointModificationAction GetMinQValAction(FModelState state);



};
