// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSLaneSpline.h"
#include "RTSResourceNode.h"
#include "Components/SplineComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

ARTSLaneSpline::ARTSLaneSpline()
{
	PrimaryActorTick.bCanEverTick = false;

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	SetRootComponent(Spline);
	Spline->SetMobility(EComponentMobility::Static);
	Spline->SetDrawDebug(false);

	HighlightSegments = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HighlightSegments"));
	HighlightSegments->SetupAttachment(Spline);
	HighlightSegments->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HighlightSegments->SetCastShadow(false);
	HighlightSegments->SetHiddenInGame(true);
	HighlightSegments->SetMobility(EComponentMobility::Movable);

	HighlightEnds = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HighlightEnds"));
	HighlightEnds->SetupAttachment(Spline);
	HighlightEnds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HighlightEnds->SetCastShadow(false);
	HighlightEnds->SetHiddenInGame(true);
	HighlightEnds->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	if (CubeMesh.Succeeded())
	{
		HighlightSegments->SetStaticMesh(CubeMesh.Object);
	}
	if (SphereMesh.Succeeded())
	{
		HighlightEnds->SetStaticMesh(SphereMesh.Object);
	}
	if (ShapeMat.Succeeded())
	{
		HighlightSegments->SetMaterial(0, ShapeMat.Object);
		HighlightEnds->SetMaterial(0, ShapeMat.Object);
	}

	// Default lane along +Y (matches camera yaw -90: "into the screen")
	Spline->ClearSplinePoints(false);
	Spline->AddSplinePoint(FVector(0.f, 0.f, 0.f), ESplineCoordinateSpace::Local, false);
	Spline->AddSplinePoint(FVector(0.f, 2000.f, 0.f), ESplineCoordinateSpace::Local, true);
	Spline->UpdateSpline();
}

void ARTSLaneSpline::BeginPlay()
{
	Super::BeginPlay();
	EnsureHighlightAssets();
	ClearHighlightMeshes();
}

void ARTSLaneSpline::EnsureHighlightAssets()
{
	if (bHighlightAssetsReady)
	{
		return;
	}

	UMaterialInterface* BaseMat = nullptr;
	if (HighlightSegments)
	{
		BaseMat = HighlightSegments->GetMaterial(0);
	}
	if (!BaseMat && HighlightEnds)
	{
		BaseMat = HighlightEnds->GetMaterial(0);
	}

	if (BaseMat)
	{
		HighlightMID = UMaterialInstanceDynamic::Create(BaseMat, this);
		if (HighlightMID)
		{
			// BasicShapeMaterial exposes "Color"
			HighlightMID->SetVectorParameterValue(TEXT("Color"), HighlightColor);
			if (HighlightSegments)
			{
				HighlightSegments->SetMaterial(0, HighlightMID);
			}
			if (HighlightEnds)
			{
				HighlightEnds->SetMaterial(0, HighlightMID);
			}
		}
	}

	bHighlightAssetsReady = true;
}

void ARTSLaneSpline::SetHighlighted(bool bInHighlighted)
{
	if (bHighlighted == bInHighlighted)
	{
		return;
	}

	bHighlighted = bInHighlighted;
	if (bHighlighted)
	{
		RebuildHighlightMeshes();
	}
	else
	{
		ClearHighlightMeshes();
	}
}

void ARTSLaneSpline::ClearHighlightMeshes()
{
	if (HighlightSegments)
	{
		HighlightSegments->ClearInstances();
		HighlightSegments->SetHiddenInGame(true);
	}
	if (HighlightEnds)
	{
		HighlightEnds->ClearInstances();
		HighlightEnds->SetHiddenInGame(true);
	}
}

void ARTSLaneSpline::RebuildHighlightMeshes()
{
	EnsureHighlightAssets();
	ClearHighlightMeshes();

	if (!Spline || !HighlightSegments || !HighlightEnds)
	{
		return;
	}

	const float Len = GetSplineLength();
	if (Len < 1.f)
	{
		return;
	}

	if (HighlightMID)
	{
		HighlightMID->SetVectorParameterValue(TEXT("Color"), HighlightColor);
	}

	const FVector ZOff(0.f, 0.f, HighlightZOffset);
	constexpr float Step = 80.f;
	const float WidthScale = FMath::Max(HighlightThickness, 8.f) / 100.f;
	const float HeightScale = FMath::Max(HighlightThickness * 0.25f, 4.f) / 100.f;

	FVector Prev = GetLocationAtDistance(0.f) + ZOff;
	for (float D = Step; D <= Len + KINDA_SMALL_NUMBER; D += Step)
	{
		const float Dist = FMath::Min(D, Len);
		const FVector Next = GetLocationAtDistance(Dist) + ZOff;
		const FVector Delta = Next - Prev;
		const float SegLen = Delta.Size();
		if (SegLen > 1.f)
		{
			const FVector Mid = (Prev + Next) * 0.5f;
			const FRotator Rot = Delta.Rotation();
			const FVector Scale(SegLen / 100.f, WidthScale, HeightScale);
			// World positions — AddInstance is local-space and would offset with the spline actor.
			HighlightSegments->AddInstanceWorldSpace(FTransform(Rot, Mid, Scale));
		}
		Prev = Next;
		if (Dist >= Len)
		{
			break;
		}
	}

	const float EndScale = FMath::Max(HighlightThickness, 20.f) / 50.f;
	const FVector EndScale3(EndScale);
	HighlightEnds->AddInstanceWorldSpace(FTransform(FRotator::ZeroRotator, GetLocationAtDistance(0.f) + ZOff, EndScale3));
	HighlightEnds->AddInstanceWorldSpace(FTransform(FRotator::ZeroRotator, GetLocationAtDistance(Len) + ZOff, EndScale3));

	HighlightSegments->SetHiddenInGame(false);
	HighlightEnds->SetHiddenInGame(false);
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
