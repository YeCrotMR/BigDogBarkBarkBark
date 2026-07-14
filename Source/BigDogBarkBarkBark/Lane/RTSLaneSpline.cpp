// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSLaneSpline.h"
#include "RTSResourceNode.h"
#include "Components/SplineComponent.h"

ARTSLaneSpline::ARTSLaneSpline()
{
	PrimaryActorTick.bCanEverTick = false;

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	SetRootComponent(Spline);
	Spline->SetMobility(EComponentMobility::Static);

	// Default lane along +Y (matches camera yaw -90: "into the screen")
	Spline->ClearSplinePoints(false);
	Spline->AddSplinePoint(FVector(0.f, 0.f, 0.f), ESplineCoordinateSpace::Local, false);
	Spline->AddSplinePoint(FVector(0.f, 2000.f, 0.f), ESplineCoordinateSpace::Local, true);
	Spline->UpdateSpline();
}

void ARTSLaneSpline::BeginPlay()
{
	Super::BeginPlay();
}

float ARTSLaneSpline::GetSplineLength() const
{
	return Spline ? Spline->GetSplineLength() : 0.f;
}

FVector ARTSLaneSpline::GetLocationAtDistance(float Distance) const
{
	if (!Spline)
	{
		return GetActorLocation();
	}
	const float Clamped = FMath::Clamp(Distance, 0.f, GetSplineLength());
	return Spline->GetLocationAtDistanceAlongSpline(Clamped, ESplineCoordinateSpace::World);
}

FRotator ARTSLaneSpline::GetRotationAtDistance(float Distance) const
{
	if (!Spline)
	{
		return GetActorRotation();
	}
	const float Clamped = FMath::Clamp(Distance, 0.f, GetSplineLength());
	return Spline->GetRotationAtDistanceAlongSpline(Clamped, ESplineCoordinateSpace::World);
}

void ARTSLaneSpline::RegisterResourceNode(ARTSResourceNode* Node)
{
	if (!Node)
	{
		return;
	}
	ResourceNodes.AddUnique(Node);
}

void ARTSLaneSpline::UnregisterResourceNode(ARTSResourceNode* Node)
{
	ResourceNodes.RemoveAll([Node](const TWeakObjectPtr<ARTSResourceNode>& Ptr)
	{
		return !Ptr.IsValid() || Ptr.Get() == Node;
	});
}

ARTSResourceNode* ARTSLaneSpline::FindNearestResourceNode(float FromDistance) const
{
	ARTSResourceNode* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();

	for (const TWeakObjectPtr<ARTSResourceNode>& WeakNode : ResourceNodes)
	{
		ARTSResourceNode* Node = WeakNode.Get();
		if (!Node)
		{
			continue;
		}
		const float Delta = FMath::Abs(Node->SplineDistance - FromDistance);
		if (Delta < BestDist)
		{
			BestDist = Delta;
			Best = Node;
		}
	}
	return Best;
}
