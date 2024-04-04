// Fill out your copyright notice in the Description page of Project Settings.


#include "SplineController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Components/SplineComponent.h"
#include "TimerManager.h"
#include "LandscapeSplinesComponent.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "LandscapeSplineSegment.h"
#include "Containers/Array.h"

// Sets default values for this component's properties
USplineController::USplineController()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}



// Called when the game starts
void USplineController::BeginPlay()
{
	Super::BeginPlay();

    generateAIPathPointPossibilities();

    if (visualizeAvailableAINavPoints) {
        visualizeAINavPoints();
    }

    // Temporary to spawn a path from the center path (could be used as the default spline path)
    //spawnAIPathingSpline(centerPath);


	// ...
    //GetWorld()->GetTimerManager().SetTimer(AddPointTimerHandle, this, &USplineController::AddPointToSpline, 1.0f, true);
	
}

// Visualizes the points that the AI can choose when creating it's navigation Spline
void USplineController::visualizeAINavPoints() {

    // Loop through all of our points
    for (FVector point : availableAINavPoints) {

        DrawDebugPoint(GetWorld(), point, 10.0f, FColor::Red, true, 5.0f);
    }
}


// Returns the average of 2 points
FVector createAveragedPoint(FVector point1, FVector point2) {

    float AvgX = (point1.X + point2.X) / 2.0f;
    float AvgY = (point1.Y + point2.Y) / 2.0f;
    float AvgZ = point1.Z;

    return FVector(AvgX, AvgY, AvgZ);
}


// Takes in one of a given segments point info and generates the average points, transforms them, and returns the array of available nav points for the AI
TArray<FVector> createTransformAndAddPoints(ULandscapeSplinesComponent* landscapeSplineComponent, FLandscapeSplineInterpPoint pointInfo) {

    TArray<FVector> points;
    // We first need to transform them into the correct world space
    points.Add(pointInfo.Center);
    points.Add(pointInfo.Left);
    points.Add(pointInfo.Right);

    // Create some additional point for higher fidelity
    points.Add(createAveragedPoint(pointInfo.Left, pointInfo.Center));
    points.Add(createAveragedPoint(pointInfo.Right, pointInfo.Center));


    TArray<FVector> adjustedPoints;
    // Loop through all the points and convert them into the proper world space, then add them to the availableAINavPoints array
    for (FVector point : points) {

        FVector WorldPoint = landscapeSplineComponent->GetComponentTransform().TransformPosition(point);
        adjustedPoints.Add(WorldPoint);
    }

    return adjustedPoints;

}


// Function to reverse the order of elements in a TArray
void ReverseArray(TArray<FLandscapeSplineInterpPoint>& Array) {
    int32 NumElements = Array.Num();
    for (int32 i = 0; i < NumElements / 2; ++i) {
        // Swap elements at opposite ends of the array
        Array.Swap(i, NumElements - 1 - i);
    }
}






// Generates a list of available points that the AI can choose from when creating the splines of it's path
// ALSO generates the perfect center path (which can be used as the default path)
void USplineController::generateAIPathPointPossibilities() {


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

                    centerPath.Reset();

                    // Need to make this weird de-referencing array to add the segments because otherwise they are protected and we can't work with them
                    TArray< TWeakObjectPtr<ULandscapeSplineSegment> > Segments;
                    for (TObjectIterator<ULandscapeSplineSegment> Itr; Itr; ++Itr)
                    {
                        Segments.Add(TWeakObjectPtr<ULandscapeSplineSegment>(*Itr));
                    }

                    // The segments in the map are originally out of order, so they are re-ordered here to make the spline connections and paths smooth
                    TArray<int32> segmentOrders = {0, 16, 15, 13, 7, 1, 10, 19, 5, 14, 4, 3, 12, 8, 17};
                    TArray<TWeakObjectPtr<ULandscapeSplineSegment>> ReOrdered = {Segments[0], Segments[16], Segments[15], Segments[13], Segments[7],
                    Segments[1], Segments[10], Segments[19], Segments[5], Segments[14], Segments[4], Segments[3], Segments[12], Segments[8], Segments[17]
                    };
                    TArray<int32> segmentsToBeInverted = { 7, 1, 10, 19, 5, 14, 4, 3, 12};
                    // So we can do array inverting for specific segments
                    int32 currSegmentIndex = 0;

                    FVector prevPointInSpline = FVector(0, 0, 0);

                    // Iterate over all landscape spline segments
                    for (TWeakObjectPtr<ULandscapeSplineSegment>& SegmentPtr : ReOrdered)
                    {
                        // Check if it exist
                        ULandscapeSplineSegment* Segment = SegmentPtr.Get();
                        if (!Segment)
                        {
                            continue;
                        }

                        TArray<FLandscapeSplineInterpPoint> segmentPointInfo = Segment->GetPoints();

                        if (segmentsToBeInverted.Contains(segmentOrders[currSegmentIndex])) {
                            ReverseArray(segmentPointInfo);
                        }


                        TArray<FVector> allPointsInThisSegment;

                        // For each point info in this segment
                        for (FLandscapeSplineInterpPoint pointInfo : segmentPointInfo) {


                            TArray<FVector> pointsForThisPointInfo = createTransformAndAddPoints(SplinesComponent, pointInfo);

                            // Append this array to our total available points array
                            availableAINavPoints.Append(pointsForThisPointInfo);
                            allPointsInThisSegment.Append(pointsForThisPointInfo);

                            // Also add to the center path
                            FVector centerPointWorld = SplinesComponent->GetComponentTransform().TransformPosition(pointInfo.Center);

                            // Also, some of the segments are duplicated......ugh
                            if (!centerPath.Contains(centerPointWorld)) {

                                centerPath.Add(centerPointWorld);
                            }
                        }

                        availableAINavPointsBySegment.Add(allPointsInThisSegment);

                        currSegmentIndex = currSegmentIndex + 1;
                    }

                  
                }
                firstComponent = false;
            }
        }
    }

}










void USplineController::AddPointToSpline() {


    // Get reference to the player character
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();

    if (PlayerController)
    {
        // Get reference to the player character controlled by this player controller
        APawn* PlayerPawn = PlayerController->GetPawn();
        if (PlayerPawn)
        {


            AActor* OwnerActor = GetOwner();


            // Use GetComponentByClass to get the spline component
            USplineComponent* SplineComponent = OwnerActor->GetComponentByClass<USplineComponent>();



            if (SplineComponent)
            {
                // GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Hello0"));
                 // Now you have a reference to the spline component, you can modify it as needed
                 // For example, adding a point to the spline component at the player character's location
                FVector PlayerLocation = PlayerPawn->GetActorLocation();

                FVector LocalPlayerLocation = SplineComponent->GetComponentTransform().InverseTransformPosition(PlayerLocation);

                FSplinePoint myPoint;
                myPoint.Position = LocalPlayerLocation;
                myPoint.Rotation = FRotator(0.0f, 0.0f, 0.0f);
                myPoint.Scale = FVector(1.0f, 1.0f, 1.0f);
                // VERY IMPORTANT
                myPoint.Type = ESplinePointType::Constant;


                SplineComponent->AddPoint(myPoint, true);

                // Remove the previous
                int32 numSplinePoints = SplineComponent->GetNumberOfSplinePoints() - 1;

                if (numSplinePoints > 2) {
                    SplineComponent->RemoveSplinePoint(0);
                }

     
            }
        }
    }
}




///////////// NEW STUFF FROM TONIGH T/////////////
///////////// NEW STUFF FROM TONIGH T/////////////
///////////// NEW STUFF FROM TONIGH T/////////////
// note: need to break up these files into smaller bits //




void USplineController::spawnAIPathingSpline(TArray<FVector> points) {



    AActor* OwnerActor = GetOwner();

    // Use GetComponentByClass to get the spline component
    USplineComponent* SplineComponent = OwnerActor->GetComponentByClass<USplineComponent>();

    if (SplineComponent)
    {

        SplineComponent->SetSplinePoints(points, ESplineCoordinateSpace::World);

        // Manually set all point types (might be a better way to do this)
        int32 numSplinePoints = SplineComponent->GetNumberOfSplinePoints();

        for (int32 i = 0; i < points.Num(); ++i) {
            SplineComponent->SetSplinePointType(i, ESplinePointType::Curve);
        }


    }


}





// Get's the available nav AI points for the various AI models that need them
TArray<FVector> USplineController::getAvailableAINavPoints() {
    return availableAINavPoints;
}

// Get's the available nav AI points for the various AI models that need them
TArray<TArray<FVector>> USplineController::getAvailableAINavPointsBySegment() {
    return availableAINavPointsBySegment;
}



///////////// NEW STUFF FROM TONIGH T/////////////
///////////// NEW STUFF FROM TONIGH T/////////////
///////////// NEW STUFF FROM TONIGH T/////////////



// Called every frame
void USplineController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    
}

