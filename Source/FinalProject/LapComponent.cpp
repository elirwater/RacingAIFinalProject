// Fill out your copyright notice in the Description page of Project Settings.


#include "LapComponent.h"
#include "LandscapeSplinesComponent.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "LandscapeSplineSegment.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Engine/StaticMeshActor.h"




// Sets default values for this component's properties
ULapComponent::ULapComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void ULapComponent::BeginPlay()
{
	Super::BeginPlay();

    generateAIBoundingBoxes();
	// ...
    setStartLine();

    // Visualizes all bounding boxes
    if (visualizeAllBoundingBoxes) {
        visualizeAllAIBoundingBoxes();
    }
}




// Called every frame
void ULapComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Visualizes the current bounding box the AI is within each tick
    if (visualizeCurrentBoundingBox) {
        visualizeCurrentAIBoundingBox();
    }

    isAIOnStartLine();
  
    UpdateAILapState();

}


// Set Start Line Static Mesh Component
// Could probably do this through the editor but I'm not sure how
void ULapComponent::setStartLine() {

    for (TActorIterator<AStaticMeshActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
    {
        AStaticMeshActor* Actor = *ActorItr;
        if (Actor)
        {
            // Check if the actor's name matches "StartLine"
            if (Actor->GetActorNameOrLabel() == "StartLine")
            {
                // Access the Static Mesh Component of the actor
                UStaticMeshComponent* MeshComponent = Actor->GetStaticMeshComponent();
                if (MeshComponent)
                {
                    startLine = MeshComponent;
                }
            }
        }
    }
}


void ULapComponent::visualizeAllAIBoundingBoxes() {

    for (FBox boundingBox : AIBoundingBoxes) {
        DrawDebugBox(GetWorld(), boundingBox.GetCenter(), boundingBox.GetExtent(), FColor::Blue, true, 0, 10);
    }

    // Visualize start line
    if (startLine) {
        FBox startLineBoundingBox = startLine->Bounds.GetBox();
        DrawDebugBox(GetWorld(), startLineBoundingBox.GetCenter(), startLineBoundingBox.GetExtent(), FColor::Blue, true, 0, 10);
    }

}


// Checks if a point is inside a bounding box and ignores the Z coordinate
bool IsPointInsideBox(const FVector& Point, const FBox& Box)
{
    return Point.X >= Box.Min.X && Point.X <= Box.Max.X &&
        Point.Y >= Box.Min.Y && Point.Y <= Box.Max.Y;
}


void ULapComponent::visualizeCurrentAIBoundingBox() {

    for (FBox boundingBox : AIBoundingBoxes) {
        if (IsPointInsideBox(AICarLocation, boundingBox)) {
            DrawDebugBox(GetWorld(), boundingBox.GetCenter(), boundingBox.GetExtent(), FColor::Green, false, -1, 0, 55);
        }
    }

    // Visualize if AI is intersecting with start line
    if (startLine) {
        FBox startLineBoundingBox = startLine->Bounds.GetBox();
        if (IsPointInsideBox(AICarLocation, startLineBoundingBox)) {
            //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("On Start Line"));
            DrawDebugBox(GetWorld(), startLineBoundingBox.GetCenter(), startLineBoundingBox.GetExtent(), FColor::Green, false, -1, 0, 55);
        }
    }
}




void ULapComponent::generateAIBoundingBoxes() {
    bool firstComponent = true;

    // Iterate through all Actors
    for (TActorIterator<AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
    {
        AActor* Actor = *ActorItr;
        if (Actor)
        {
            // Check if the actor has a LandscapeSplinesComponent
            ULandscapeSplinesComponent* SplinesComponent = Actor->FindComponentByClass<ULandscapeSplinesComponent>();
            if (SplinesComponent)
            {
                // Silly thing so we can avoid the first constructor spline segment that I can't seem to delete or find
                if (!firstComponent) {


                    // Need to make this weird de-referencing array to add the segments because otherwise they are protected and we can't work with them
                    TArray< TWeakObjectPtr<ULandscapeSplineSegment> > Segments;
                    for (TObjectIterator<ULandscapeSplineSegment> Itr; Itr; ++Itr)
                    {
                        Segments.Add(TWeakObjectPtr<ULandscapeSplineSegment>(*Itr));
                    }

                    for (TWeakObjectPtr<ULandscapeSplineSegment>& SegmentPtr : Segments)
                    {
                        // Check if it exist
                        ULandscapeSplineSegment* Segment = SegmentPtr.Get();
                        if (!Segment)
                        {
                            continue;
                        }

                        // We grab the boundaries for this segment of the landscape spline
                        FBox SplineBounds = Segment->GetBounds();


                        // Transform the box to world space
                        FVector WorldCenter = SplinesComponent->GetComponentTransform().TransformPosition(SplineBounds.GetCenter());
                        FVector BoxExtent = SplineBounds.GetExtent();

                        FBox AdjustedBox(WorldCenter - BoxExtent, WorldCenter + BoxExtent);

                        AIBoundingBoxes.Add(AdjustedBox);
                    }

                }
                firstComponent = false;
            }
        }
    }
}


// Controls a variable called "isAIOnStartLine" and assings whether or not this is true
// CALLED EVERY TICK
void ULapComponent::isAIOnStartLine() {
    if (startLine) {

        FBox startLineBoundingBox = startLine->Bounds.GetBox();
        if (IsPointInsideBox(AICarLocation, startLineBoundingBox)) {
            isAIOnStartLineBool = true;
        }
        else {
            isAIOnStartLineBool = false;
        }
    }
}


// Updates the state of the lap (isLapInProgress var) based on whether the AI has crossed the started the lap (by crossing the start line)
// or finished the lap (by crossing the start line)
void ULapComponent::UpdateAILapState()
{
    // Store the current state in a temporary variable
    bool bIsAIOnStartLineNow = isAIOnStartLineBool;


    // Check if the state has changed from true to false (while the lap hasn't started) -- the lap started
    if (bWasAIOnStartLine && !bIsAIOnStartLineNow && (LapState.State == ELapState::LapNotStarted))
    {
        // Transition from true to false occurred - this means the AI has left the start line
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("LAP IS IN PROGRESS"));
        LapState.State = ELapState::LapInProgress;
    }

    // Check if the state has changed from false to true (while the lap has already been started) -- the lap ended
    if (!bWasAIOnStartLine && bIsAIOnStartLineNow && (LapState.State == ELapState::LapInProgress)) {
        // Transition from false to true occurred - this means the AI has left the start line
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("LAP ENDED"));
        LapState.State = ELapState::LapEnded;

        // Handle the completed lap stuff
        CompleteLap();
    }


    // Update the previous state variable for the next tick
    bWasAIOnStartLine = bIsAIOnStartLineNow;
}


// Completes the lap, and sets the lap time, and broadcasts to the delegate (telling the controller the lap has been completed and which calls the HandleLapCompleted function)
void ULapComponent::CompleteLap()
{
    if (LapState.State == ELapState::LapEnded)
    {
        // Calculate lap time
        float LapTime = GetWorld()->TimeSeconds - StartTime;

        // Print lap time when lap ends
        FString LapTimeString = FString::Printf(TEXT("LAP TIME: %.2f seconds"), LapTime);
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, LapTimeString);

        // Stop the timer
        GetWorld()->GetTimerManager().ClearTimer(LapTimerHandle);

        //Triggers the execution of all functions bound to the delegate (which is the HandleLapCompleted function in the SimulationController)
        OnLapCompletedDelegate.Broadcast();
    }
}


// Runs the execution of 1 AI Lap
void ULapComponent::RunLap() {

    if (AICarPawn && startLine) {

        // We set the lap state to not started
        LapState.State = ELapState::LapNotStarted;

        // Then we spawn teleport the AI car to the correct location on the start line
        AICarPawn->SetActorLocation(startLine->Bounds.GetBox().GetCenter(), false, nullptr, ETeleportType::TeleportPhysics);

        // THen we set the start time
        StartTime = GetWorld()->TimeSeconds;

        // Because the car will automatically follow the spline, the other functions are responsible for timing and completing the lap

    }
}



