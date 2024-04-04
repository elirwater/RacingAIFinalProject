// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
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

	// Runs the AI through 1 lap
	void RunLap();


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

	// Enum for the different states a lap could be in 
	enum class ELapState : uint8
	{
		LapNotStarted UMETA(DisplayName = "Lap Not Started"),
		LapInProgress UMETA(DisplayName = "Lap In Progress"),
		LapEnded UMETA(DisplayName = "Lap Ended")
	};

	struct FLapState
	{

		UPROPERTY(BlueprintReadWrite, Category = "Lap State")
		ELapState State;

		// Default constructor
		FLapState() : State(ELapState::LapNotStarted) {}

		// Constructor with initial state
		FLapState(ELapState InitialState) : State(InitialState) {}

		// Equality operator to compare two FLapState instances
		bool operator==(const FLapState& Other) const
		{
			return State == Other.State;
		}

		// Inequality operator to compare two FLapState instances
		bool operator!=(const FLapState& Other) const
		{
			return State != Other.State;
		}
	};


	// Stores the current state of the lap
	FLapState LapState;



	// Stores the current state of whether or not the AI is on the start line
	bool isAIOnStartLineBool = false;

	// Stores the previous state of the AI on the start line
	bool bWasAIOnStartLine = false;


	// Controls a variable called "isAIOnStartLine" and assings whether or not this is true
	void isAIOnStartLine();

	// Updates the state of the lap (isLapInProgress var) based on whether the AI has crossed the started the lap (by crossing the start line)
	// or finished the lap (by crossing the start line)
	void UpdateAILapState();

	// Completes the lap, and sets the lap time, and broadcasts to the delegate (telling the controller the lap has been completed and which calls the HandleLapCompleted function)
	void CompleteLap();

	// The start time for 
	float StartTime;

	// This lap time
	float lapTime;

	// A timer for a given lap
	FTimerHandle LapTimerHandle;

	// We create a delegate OnLapCompletedDelegate which we can broadcast to, this delegate is bound to a function in the controller called 
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLapCompletedDelegate);
	UPROPERTY(BlueprintAssignable)
	FOnLapCompletedDelegate OnLapCompletedDelegate;
};
