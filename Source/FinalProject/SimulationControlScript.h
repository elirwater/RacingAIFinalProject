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
	void HandleLapCompleted();
	
	// Calls the Spline Controller to actually generate the points the AI model selects for this iteration
	void generateSplinePoints(TArray<FVector> points);

	// Reference to the Spline that is set in the blueprint for this class
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actors")
	USplineController* SplineComponent;



};
