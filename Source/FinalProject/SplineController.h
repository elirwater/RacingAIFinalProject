// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SplineController.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FINALPROJECT_API USplineController : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USplineController();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// A list of all the points that the spline can conform to
	TArray<FVector> availableAINavPoints;

	// The center path / racing line for the points
	TArray<FVector> centerPath;

	// Which points exist for each segment
	TArray<TArray<FVector>> availableAINavPointsBySegment;

	// Generates a list of available points that the AI can choose from when creating the splines of it's path
	// ALSO generates the perfect center path (which can be used as the default path)
	void GenerateAIPathPointPossibilities();

	// Visiualizes all the points avaialble for the spline to conform to
	void visualizeAINavPoints();

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void AddPointToSpline();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool visualizeAvailableAINavPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	int32 visualizeSpecificSegment;

	// Returns the available Nav points (in better protected OOD style - I didnt' have time to do this for everything)
	TArray<FVector> getAvailableAINavPoints();

	TArray<TArray<FVector>> getAvailableAINavPointsBySegment();

	// Actually spawn the spline into the world given a list of points
	void SpawnAIPathingSpline(TArray<FVector> points);
		
};
