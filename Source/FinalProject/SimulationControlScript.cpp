// Fill out your copyright notice in the Description page of Project Settings.


#include "SimulationControlScript.h"
#include "TimerManager.h"

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


void USimulationControlScript::runLap() {

	if (AILapComponent) {
		AILapComponent->RunLap();
	}
}



// Called every frame
void USimulationControlScript::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//// Or whereever we want to call this.
	// ...

	
}

