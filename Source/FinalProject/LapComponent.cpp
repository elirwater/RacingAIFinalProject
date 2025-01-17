// Fill out your copyright notice in the Description page of Project Settings.


#include "LapComponent.h"
#include "LandscapeSplinesComponent.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "LandscapeSplineSegment.h"
#include <float.h>
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

    OutOfBoundsChecker();

    OverTimeChecker();

    SpeedChecker();

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

        // Handle the completed lap stuff
        CompleteLap();
    }

    // Update the previous state variable for the next tick
    bWasAIOnStartLine = bIsAIOnStartLineNow;
}


// Completes the lap, and sets the lap time, and broadcasts to the delegate (telling the controller the lap has been completed and which calls the HandleLapCompleted function)
void ULapComponent::CompleteLap()
{

    LapState.State = ELapState::LapEnded;

    // Calculate lap time
    lapTime = GetWorld()->GetTimeSeconds() - StartTime;

    // Print lap time when lap ends
    FString LapTimeString = FString::Printf(TEXT("LAP TIME: %.2f seconds"), lapTime);
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, LapTimeString);

    // Stop the timer
    GetWorld()->GetTimerManager().ClearTimer(LapTimerHandle);

    //Triggers the execution of all functions bound to the delegate (which is the HandleLapCompleted function in the SimulationController)
    OnLapCompletedDelegate.Broadcast();
}


// Completes the lap with a fail, passing back to the controller a super high time value 
// This is either called by the OutOfBoundsChecker if the car goes out of the available segments
// OR if the lap time is greater than 1 min
void  ULapComponent::FailLap() {

    LapState.State = ELapState::LapEnded;

    // Set the lap time to be really high
    lapTime = MAX_FLT;

    // Print lap time when lap ends
    FString LapTimeString = FString::Printf(TEXT("LAP TIME: %.2f seconds"), lapTime);
    GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Green, LapTimeString);

    // Stop the timer
    GetWorld()->GetTimerManager().ClearTimer(LapTimerHandle);

    //Triggers the execution of all functions bound to the delegate (which is the HandleLapCompleted function in the SimulationController)
    OnLapCompletedDelegate.Broadcast();


}


// Called every tick to check if the AI has wandered out of bounds, if so, the lap is failed
void ULapComponent::OutOfBoundsChecker() {
    
    if (LapState.State == ELapState::LapInProgress) {
        for (FBox boundingBox : AIBoundingBoxes) {
            if (IsPointInsideBox(AICarLocation, boundingBox)) {
                // If the AI Vehicle is inside a box, we return and nothing happens
                return;
            }
        }
        FString LapTimeString = FString::Printf(TEXT("OUT OF BOUNDS CHECKER FAIL"));
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, LapTimeString);
        // If we made it here, the AI was outside of the bounding boxes so the lap is failed
        FailLap();
    }
}

// Called every tick to check if the AI has gone over it's alloted time, if so, the lap is failed
void ULapComponent::OverTimeChecker() {

    lapTime = GetWorld()->GetTimeSeconds() - StartTime;

    if (lapTime > 45.f) {
        FString LapTimeString = FString::Printf(TEXT("OVERTIME FAIL"));
        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, LapTimeString);
        FailLap();
    }

}

// Called every tick to check if the AI is stopped
void ULapComponent::SpeedChecker() {

    FVector CarVelocity = AICarPawn->GetVelocity();

    // Get the forward vector of the AICarPawn
    FVector CarForwardVector = AICarPawn->GetActorForwardVector();

    // Project the velocity onto the forward vector
    float ForwardSpeed = FVector::DotProduct(CarVelocity, CarForwardVector);

    if (LapState.State == ELapState::LapInProgress) {
        if (ForwardSpeed <= 10) {
            FString LapTimeString = FString::Printf(TEXT("SPEED FAIL - CAR IS STOPPED"));
            GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, LapTimeString);
            FailLap();
        }
    }
}


// Runs the execution of 1 AI Lap
void ULapComponent::RunLap() {

    if (AICarPawn && startLine) {

        // We set the lap state to not started
        LapState.State = ELapState::LapNotStarted;

        // Then we spawn teleport the AI car to the correct location on the start line

        lapNumber += 1;
        FString LapTimeString = FString::Printf(TEXT("COMMENCING LAP #  %.2f"), lapNumber);
        GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Purple, LapTimeString);
        //AICarPawn->setWorldLocationAndRota(startLine->Bounds.GetBox().GetCenter(), false, nullptr, ETeleportType::TeleportPhysics);


        // Stop physics simulation for the AI car pawn (otherwise it will have a ton of velocity and weird momentum after it teleports)
        if (AICarPawn->GetRootComponent())
        {
            UPrimitiveComponent* RootPrimitiveComponent = Cast<UPrimitiveComponent>(AICarPawn->GetRootComponent());
            if (RootPrimitiveComponent)
            {
                RootPrimitiveComponent->SetSimulatePhysics(false);
            }
        }

        // Teleport the AI car pawn to the center of the start line's bounding box without simulating physics
        AICarPawn->SetActorLocation(startLine->Bounds.GetBox().GetCenter(), false);

        FQuat rotation = FQuat::Identity;
        AICarPawn->SetActorRotation(rotation, ETeleportType::ResetPhysics);
        AICarPawn->SetActorLocation(startLine->Bounds.GetBox().GetCenter(), false);
        

        // Reset physics properties after teleportation
        if (AICarPawn->GetRootComponent())
        {
            UPrimitiveComponent* RootPrimitiveComponent = Cast<UPrimitiveComponent>(AICarPawn->GetRootComponent());
            if (RootPrimitiveComponent)
            {
                // Reset physics properties as needed
                RootPrimitiveComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
                RootPrimitiveComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

                // Restart physics simulation
                RootPrimitiveComponent->SetSimulatePhysics(true);
            }
        }
        // Then we set the start time
        StartTime = GetWorld()->GetTimeSeconds();
    }
}



