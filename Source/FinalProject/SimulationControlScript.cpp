// Fill out your copyright notice in the Description page of Project Settings.


#include "SimulationControlScript.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"



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

	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f); // Set simulation speed to 20x


	FTimerHandle TimerHandle;
	float DelaySeconds = 1.0f; // Adjust as needed
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &USimulationControlScript::runLap, DelaySeconds, false);

}


TArray<int32> findCenterPointIndexesForThisSegment(TArray<TArray<FVector>> availableAINavPointsBySegment, int32 segmentIdx) {
	TArray<int32> centerPointIndexesForThisSegment;

	int32 j = 0;
	int32 segmentStartingIdx = 0;
	for (TArray<FVector> segment : availableAINavPointsBySegment) {
		if (segmentIdx == j) {
			for (int32 i = 0; i < segment.Num(); i += 5)
			{
				if (i + 5 <= segment.Num()) {
					centerPointIndexesForThisSegment.Add(i + segmentStartingIdx);
				}
			}
		}

		segmentStartingIdx = segmentStartingIdx + segment.Num();
		j = j + 1;
	}
	return centerPointIndexesForThisSegment;
}



void USimulationControlScript::runLap() {


	TArray<FVector> modelGeneratedPoints;
	int currSegmentIdx = 0;
	TArray<TArray<FVector>> availableAINavPointsBySegment;

	// We begin each episode / epoch in the controller by grabbing the available points by segment
	if (SplineComponent) {
		availableAINavPointsBySegment = SplineComponent->getAvailableAINavPointsBySegment();
	}


	// Then we run one episode / epoch  of the Reinforcent Learning Model and gather the points it generates for this particular segment
	if (ReinforcementLearningAI) {

		// If we haven't initalized the current state, we are on the first epoch for this segment
		if (!currentState.IsInitialized()) {

			// We initialize the current state as the center points for each point row in this segment
			TArray<int32> centerPointsForThisSegment;
			for (int32 i = 0; i < availableAINavPointsBySegment[currSegmentIdx].Num(); i += 5)
			{
				if (i + 5 <= availableAINavPointsBySegment[currSegmentIdx].Num()) {
					centerPointsForThisSegment.Add(i);
				}
			}
			// Initialize the starting state
			currentState.SetPointIndexes(centerPointsForThisSegment);

			// We now create a list of indices that correspond to our available points for this segment
			for (int32 Index = 0; Index < availableAINavPointsBySegment[currSegmentIdx].Num(); ++Index) {
				availablePointIndiciesForSegment.Add(Index);
			}
		}


		// Select an action using epsilon-greedy policy
		selectedAction = ReinforcementLearningAI->EpsilonGreedyPolicyGenerateAction(currentState, 0.1, availablePointIndiciesForSegment);


		// Now we perform the action by first modifying the state and then generating the spline and running it
		nextState = currentState;

		// We find the index in our current state we want to replace
		int32 indexToReplaceInCurrentState = selectedAction.PreviousPointIndex / 5;

		nextState.PointIndexes[indexToReplaceInCurrentState] = selectedAction.NewPointIndex;


		for (int32 i = 0; i < nextState.PointIndexes.Num(); i+=1)
		{
			modelGeneratedPoints.Add(availableAINavPointsBySegment[currSegmentIdx][nextState.PointIndexes[i]]);
		}
	}

	// Then we fill in the rest of the points with the centerline points for the other segments, but keep the model generated points for this segment
	// and generate the corresponding spline
	TArray<FVector> allPoints;
	int32 j = 0;
	for (TArray<FVector> segment : availableAINavPointsBySegment) {
		if (currSegmentIdx == j) {
			allPoints.Append(modelGeneratedPoints);
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

