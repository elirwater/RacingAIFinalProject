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
}


// Called when the game starts
void USimulationControlScript::BeginPlay()
{
	Super::BeginPlay();

	// For instantiation reasons, we can't call the start the simulation until 1 second so every other component has time to set itself up
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &USimulationControlScript::SetupSimulationInfrastructure, 1, false);
}

// Finds the indicies for the points for a given racing line by segment
TArray<int32> findRacingLineForThisSegment(TArray<TArray<FVector>> availableAINavPointsBySegment, int32 segmentIdx, int32 racingLineIdx) {
	
	TArray<int32> pointIndexesForThisSegmentRacingLine;
	// Loop through all points by segment, and offset by the racing line index for the indices of the points used for this segment
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


void USimulationControlScript::SetupSimulationInfrastructure() {

	if (SplineComponent) {
		// We grab all available points first
		availableAINavPointsBySegment = SplineComponent->getAvailableAINavPointsBySegment();
	}

	// Populate our points by segment array with middle points
	int j = 0;
	for (TArray<FVector> segment : availableAINavPointsBySegment) {
		PointsToUseBySegment.Add({});
		for (int32 i = 0; i < segment.Num(); i += 5)
		{
			if (i + 5 <= segment.Num()) {
				PointsToUseBySegment[j].Add(segment[i]);
			}
		}
		j += 1;
	}

	// Start our first lap
	runLap();
}


void USimulationControlScript::runLap() {

	// If the current epoch for this segment group has ended....
	if (currentEpoch >= epochsPerGroup) {
		FString stringToUse = FString::Printf(TEXT("END OF EPOCHS FOR THIS SEGMENT GROUP - RESETTING AND TAKING THE BEST POINTS FOR THIS SEGMENT GROUP"));
		GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Blue, stringToUse);

		// We find the best state by finding the best action from any state - and we set it as this segment groups final points - and then we move on to the next segment group
		FState bestActionState = ReinforcementLearningAI->GetBestState();
		TArray<FVector> bestStatePoints = {};

		// For each segment in this segment group...
		for (FRacingLineForSegment line : bestActionState.SegmentRacingLines) {
			PointsToUseBySegment[line.SegmentIndex] = {};

			// We find the indicies for this segment's best racing line...
			TArray<int32> modelGeneratedPointsIndexesForThisSegment = findRacingLineForThisSegment(availableAINavPointsBySegment, line.SegmentIndex, line.RacingLineIndex);
			for (int32 pointIndex : modelGeneratedPointsIndexesForThisSegment) {
				
				PointsToUseBySegment[line.SegmentIndex].Add(availableAINavPointsBySegment[line.SegmentIndex][pointIndex]);
				bestStatePoints.Add(availableAINavPointsBySegment[line.SegmentIndex][pointIndex]);
			}
		}

		// Visualize this segment group's final best points (this is what the AI will follow from now on as it works on subsequent segment groups)
		for (FVector point : bestStatePoints) {
			DrawDebugPoint(GetWorld(), point, 20.0f, FColor::Purple, true, 5.0f);
		}

		currentEpoch = 0;

		// Iterate the current segment group
		currentSegmentGroup += 1;
		
		// Reset the Q Table
		ReinforcementLearningAI->qTable = {};

		// Reset the State to uninitialized
		currentState = FState();

		// If we are out of segment groups, we pause the simulation (and mark it as over so we can look at it)
		if (currentSegmentGroup == segmentGroups.Num() - 1) {

			stringToUse = FString::Printf(TEXT("SIMULATION HAS ENDED"));
			GEngine->AddOnScreenDebugMessage(-1, 3000.f, FColor::Red, stringToUse);
			GetWorld()->GetWorldSettings()->SetPauserPlayerState(GetWorld()->GetFirstPlayerController()->PlayerState);
		}
	}

	
	// If we are still working on this segment group....
	FString stringToUse = FString::Printf(TEXT("Starting Epoch # : %d"), currentEpoch);
	GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Yellow, stringToUse);

	// Which segment group we are currently working on
	TArray<int32> currSegmentIndicies = segmentGroups[currentSegmentGroup];

	// Then we run one episode / epoch  of the Reinforcent Learning Model and gather the points it generates for this particular segment
	if (ReinforcementLearningAI) {

		// If we haven't initalized the current state, we are on the first epoch for this segment
		if (!currentState.bIsInitialized) {

			FState newState = FState(TArray<FRacingLineForSegment>{});

			for (int32 segmentIndex : currSegmentIndicies) {
				newState.SegmentRacingLines.Add(FRacingLineForSegment(segmentIndex, 0));
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

			// Wipe this segments points and replace with model generated ones
			PointsToUseBySegment[line.SegmentIndex] = {};

			TArray<int32> modelGeneratedPointsIndexesForThisSegment = findRacingLineForThisSegment(availableAINavPointsBySegment, line.SegmentIndex, line.RacingLineIndex);
			for (int32 pointIndex : modelGeneratedPointsIndexesForThisSegment) {

				PointsToUseBySegment[line.SegmentIndex].Add(availableAINavPointsBySegment[line.SegmentIndex][pointIndex]);
			}
		}

		// Let's visualize the best state-action that the algorithm has found
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


	// Now, we take our points by segment and turn it into a flattened list of points
	TArray<FVector> pointsToCreate = {};
	for (TArray<FVector> segment : PointsToUseBySegment) {
		pointsToCreate.Append(segment);
	}

	// Then we generate the spline on the track with those points
	if (SplineComponent) {
		SplineComponent->SpawnAIPathingSpline(pointsToCreate);
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

	// We update our Q table based on the lap time of that state-action pair
	if (ReinforcementLearningAI) {
		ReinforcementLearningAI->UpdateQTable(currentState, selectedAction, LapTime, nextState, 0.1, 0.9);
	}

	// iterate to the next lap / epoch
	currentState = nextState;
	currentEpoch += 1;
	runLap();
}


void USimulationControlScript::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

