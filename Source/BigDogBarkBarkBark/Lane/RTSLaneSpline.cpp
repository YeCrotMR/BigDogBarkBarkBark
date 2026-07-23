// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSLaneSpline.h"
#include "RTSResourceNode.h"
#include "Components/SplineComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

ARTSLaneSpline::ARTSLaneSpline()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

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

void ARTSLaneSpline::SetHighlighted(bool bInHighlighted)
{
	if (bHighlighted == bInHighlighted)
	{
		return;
	}
	bHighlighted = bInHighlighted;
	SetActorTickEnabled(bHighlighted);
	if (Spline)
	{
		Spline->SetDrawDebug(bHighlighted);
	}
	if (bHighlighted)
	{
		DrawHighlight();
	}
}

void ARTSLaneSpline::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bHighlighted)
	{
		DrawHighlight();
	}
}

void ARTSLaneSpline::DrawHighlight() const
{
	UWorld* World = GetWorld();
	if (!World || !Spline)
	{
		return;
	}

	const float Len = GetSplineLength();
	if (Len < 1.f)
	{
		return;
	}

	const FColor Color = HighlightColor.ToFColor(true);
	constexpr float Step = 80.f;
	FVector Prev = GetLocationAtDistance(0.f) + FVector(0.f, 0.f, 40.f);
	for (float D = Step; D <= Len; D += Step)
	{
		const FVector Next = GetLocationAtDistance(D) + FVector(0.f, 0.f, 40.f);
		DrawDebugLine(World, Prev, Next, Color, false, -1.f, 0, HighlightThickness);
		Prev = Next;
	}
	const FVector End = GetLocationAtDistance(Len) + FVector(0.f, 0.f, 40.f);
	if (!Prev.Equals(End, 1.f))
	{
		DrawDebugLine(World, Prev, End, Color, false, -1.f, 0, HighlightThickness);
	}

	// End markers so the lane reads clearly from top-down camera
	DrawDebugSphere(World, GetLocationAtDistance(0.f) + FVector(0.f, 0.f, 40.f), 35.f, 8, Color, false, -1.f, 0, 2.f);
	DrawDebugSphere(World, End, 35.f, 8, Color, false, -1.f, 0, 2.f);
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

float ARTSLaneSpline::GetDistanceForWorldLocation(const FVector& WorldLocation) const
{
	if (!Spline)
	{
		return 0.f;
	}
	const float InputKey = Spline->FindInputKeyClosestToWorldLocation(WorldLocation);
	return Spline->GetDistanceAlongSplineAtSplineInputKey(InputKey);
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
