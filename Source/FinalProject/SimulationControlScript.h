// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LapComponent.h"
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


	// executes 1 lap 
	void runLap();

};
