// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LapComponent.h"
#include "ReinforcementLearningAI.h"
#include "SimulationControlScript.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FINALPROJECT_API USimulationControlScript : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USimulationControlScript();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Reference to the AI Lap Component that is set in the blueprint for this class
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actors")
	ULapComponent* AILapComponent;


	// Reference to the Reinforceemtn Learning AI Component that is set in the blueprint for this class
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actors")
	UReinforcementLearningAI* ReinforcementLearningAI;


	// executes 1 lap 
	void runLap();

	UFUNCTION()
	// Once a lap has been completed, the delegate method is called and this function is in turn called to handle the completed lap
	void HandleLapCompleted();
	
	// Calls the Spline Controller to actually generate the points the AI model selects for this iteration
	void GenerateSplinePoints(TArray<FVector> points);

	// Reference to the Spline that is set in the blueprint for this class
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actors")
	USplineController* SplineComponent;

	// Keeps track of the current state
	FState currentState;

	// Keeps track of the next state (while the lap runs)
	FState nextState;

	// Keeps track of which action was selected
	FAction selectedAction;


	
	// Starts as the center line, after every segment training, the segment points get replaced by the best points that the RL algorithm found for this segment
	TArray<TArray<FVector>> PointsToUseBySegment;

	// Which points can be used by segment for the spline
	TArray<TArray<FVector>> availableAINavPointsBySegment;

	// Which segments we want grouped together
	TArray<TArray<int32>> segmentGroups = {
		{0, 1},
		{2, 3},
		{4, 5},
		{6, 7, 8},
		{9, 10},
		{11, 12},
		{13, 14}

	};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actors")
	// Which segment group the algorithm is currently working on
	int32 currentSegmentGroup = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actors")
	// Current epoch for this segment group
	int32 currentEpoch = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actors")
	// How many epochs there should be for each segment group
	int32 epochsPerGroup = 50;


	// Set's up everything that needs to be setup at the beginning of a simulation
	void SetupSimulationInfrastructure();


};
