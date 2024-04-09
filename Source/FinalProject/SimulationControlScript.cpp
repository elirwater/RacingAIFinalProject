// Fill out your copyright notice in the Description page of Project Settings.


#include "SimulationControlScript.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"



// Sets default values for this component's properties
USimulationControlScript::USimulationControlScript()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void USimulationControlScript::BeginPlay()
{

	Super::BeginPlay();


	FTimerHandle TimerHandle;
	float DelaySeconds = 1.0f; // Adjust as needed
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &USimulationControlScript::runLap, DelaySeconds, false);

}


TArray<int32> findRacingLineForThisSegment(TArray<TArray<FVector>> availableAINavPointsBySegment, int32 segmentIdx, int32 racingLineIdx) {
	TArray<int32> pointIndexesForThisSegmentRacingLine;

	int32 j = 0;
	for (TArray<FVector> segment : availableAINavPointsBySegment) {
		if (segmentIdx == j) {
			for (int32 i = 0; i < segment.Num(); i += 5)
			{
				if (i + 5 <= segment.Num()) {
					pointIndexesForThisSegmentRacingLine.Add(i + racingLineIdx);
				}
			}
		}
		j = j + 1;
	}
	return pointIndexesForThisSegmentRacingLine;
}



void USimulationControlScript::runLap() {


	TArray<FVector> modelGeneratedPoints;
	TArray<int32> currSegmentIndicies = TArray<int32>{0, 1};

	TArray<TArray<FVector>> availableAINavPointsBySegment;

	// We begin each episode / epoch in the controller by grabbing the available points by segment
	if (SplineComponent) {
		availableAINavPointsBySegment = SplineComponent->getAvailableAINavPointsBySegment();
	}


	// Then we run one episode / epoch  of the Reinforcent Learning Model and gather the points it generates for this particular segment
	if (ReinforcementLearningAI) {

		// If we haven't initalized the current state, we are on the first epoch for this segment
		if (!currentState.bIsInitialized) {

			FState newState = FState(TArray<FRacingLineForSegment>{});
			for (int32 i = 0; i < currSegmentIndicies.Num(); i += 1) {
				newState.SegmentRacingLines.Add(FRacingLineForSegment(i, 0));
			}
			currentState = newState;
		}


		// Select an action using epsilon-greedy policy
		selectedAction = ReinforcementLearningAI->EpsilonGreedyPolicyGenerateAction(currentState, 0.6);


		// Now we perform the action by first modifying the state and then generating the spline and running it
		nextState = currentState;


		// We update our next state
		for (FRacingLineForSegment& line : nextState.SegmentRacingLines) {

			if (selectedAction.SegmentToModify == line.SegmentIndex) {
				line.RacingLineIndex = selectedAction.NewRacingLineIndex;
			}
		}


		// Now we figure out what the racing line indexes correspond to as actual track points in space
		for (FRacingLineForSegment line : nextState.SegmentRacingLines) {
			TArray<int32> modelGeneratedPointsIndexesForThisSegment = findRacingLineForThisSegment(availableAINavPointsBySegment, line.SegmentIndex, line.RacingLineIndex);
			for (int32 pointIndex : modelGeneratedPointsIndexesForThisSegment) {
				modelGeneratedPoints.Add(availableAINavPointsBySegment[line.SegmentIndex][pointIndex]);
			}
		}


		// FOR VISUALIZATION OF THE BEST STATE -> ACTION POINTS:
		// Now we figure out what the racing line indexes correspond to as actual track points in space
		FState bestActionState = ReinforcementLearningAI->GetBestState();
		TArray<FVector> bestStatePoints;

		for (FRacingLineForSegment line : bestActionState.SegmentRacingLines) {
			TArray<int32> modelGeneratedPointsIndexesForThisSegment = findRacingLineForThisSegment(availableAINavPointsBySegment, line.SegmentIndex, line.RacingLineIndex);
			for (int32 pointIndex : modelGeneratedPointsIndexesForThisSegment) {
				bestStatePoints.Add(availableAINavPointsBySegment[line.SegmentIndex][pointIndex]);
			}
		}
		for (FVector point : bestStatePoints) {
			DrawDebugPoint(GetWorld(), point, 20.0f, FColor::Orange, false, 5.0f);
		}
	}

	// Then we fill in the rest of the points with the centerline points for the other segments, but keep the model generated points for this segment
	// and generate the corresponding spline
	TArray<FVector> allPoints;
	allPoints.Append(modelGeneratedPoints);
	int32 j = 0;
	for (TArray<FVector> segment : availableAINavPointsBySegment) {
		if (currSegmentIndicies.Contains(j)) {
			j = j + 1;
			continue;
		}
		else {
			// Iterate through the array of FVector points 5 at a time, and take the center point
			for (int32 i = 0; i < segment.Num(); i += 5)
			{
				if (i + 5 <= segment.Num()) {
					allPoints.Add(segment[i]);
				}
			}
		}
		j = j + 1;
	}

	// Then we generate the spline on the track with those points
	if (SplineComponent) {
		SplineComponent->spawnAIPathingSpline(allPoints);
	}

	// Then we actually run the lap using the Lap Component and generate it's time/score
	if (AILapComponent)
	{	
		// If our delegate is not bound to the HandleLapCompleted function yet
		if (!AILapComponent->OnLapCompletedDelegate.IsBound()) {
			// Binds the HandleLapCompleted function to the Lap Component's OnLapCompleted delegate, so when this delegate is broadcast to, it calls the HandleLapCompleted function
			AILapComponent->OnLapCompletedDelegate.AddDynamic(this, &USimulationControlScript::HandleLapCompleted);
			AILapComponent->RunLap();
		}
		else {
			AILapComponent->RunLap();
		}
	}
}


void USimulationControlScript::HandleLapCompleted()
{
	float LapTime = AILapComponent->lapTime;
	FString LapTimeString = FString::Printf(TEXT("LAP TIME FROM CONTROLLER: %.2f seconds"), LapTime);
	GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Green, LapTimeString);

	if (ReinforcementLearningAI) {
		ReinforcementLearningAI->UpdateQTable(currentState, selectedAction, LapTime, nextState, 0.1, 0.9);
	}

	currentState = nextState;



	runLap();
}


// Called every frame
void USimulationControlScript::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//// Or whereever we want to call this.
	// ...

	
}

