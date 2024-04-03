// Fill out your copyright notice in the Description page of Project Settings.


#include "LapComponent.h"
#include "LandscapeSplinesComponent.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "LandscapeSplineSegment.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
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



int32 ULapComponent::RunLap() {

    // First, we spawn teleport the AI car to the correct location on the start line
    if (AICarPawn && startLine) {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Hello"));
        AICarPawn->SetActorLocation(startLine->Bounds.GetBox().GetCenter(), false, nullptr, ETeleportType::TeleportPhysics);

    }



	//FString DebugMessage = FString::Printf(TEXT("Vehicle Location: X=%.2f, Y=%.2f, Z=%.2f"), AICarLocation.X, AICarLocation.Y, AICarLocation.Z);
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, DebugMessage);


	return 1;
}



