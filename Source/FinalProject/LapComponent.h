// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LapComponent.generated.h"

 
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FINALPROJECT_API ULapComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULapComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Runs the AI through 1 lap and returns the time (if the time is -1 then the car went out of bounds)
	int32 RunLap();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actors")
	FVector AICarLocation = FVector(0, 0, 0);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	APawn* AICarPawn;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actors")
	UStaticMeshComponent* startLine;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool visualizeCurrentBoundingBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool visualizeAllBoundingBoxes;




	//  bounding boxes for where the AI is allowed to drive into
	TArray<FBox> AIBoundingBoxes;


	// Generates the bounding boxes for where the AI is allowed to drive into based on the landscape spline's bounding boxes
	void generateAIBoundingBoxes();

	// Visualizes all bounding boxes
	void visualizeAllAIBoundingBoxes();

	// Visualizes the current bounding box the AI is within
	void visualizeCurrentAIBoundingBox();

	// Set Start Line Static Mesh Component
	void setStartLine();
		
};
