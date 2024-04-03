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

	FTimerHandle AddPointTimerHandle;

	FVector prevSplinePoint;

	TArray<FVector> availableAINavPoints;

	TArray<FVector> centerPath;


	void generateAIPathPointPossibilities();

	void visualizeAINavPoints();

	void spawnAIPathingSpline(TArray<FVector> points);



public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void AddPointToSpline();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool visualizeAvailableAINavPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	int32 visualizeSpecificSegment;


		
};
