// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSResourceNode.h"
#include "RTSLaneSpline.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ARTSResourceNode::ARTSResourceNode()
{
	PrimaryActorTick.bCanEverTick = false;

	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
	SetRootComponent(MarkerMesh);
	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMesh.Succeeded())
	{
		MarkerMesh->SetStaticMesh(CubeMesh.Object);
		MarkerMesh->SetWorldScale3D(FVector(0.5f, 0.5f, 0.5f));
	}
}

void ARTSResourceNode::BeginPlay()
{
	Super::BeginPlay();
	SnapToLane();
	if (OwnerLane)
	{
		OwnerLane->RegisterResourceNode(this);
	}
}

void ARTSResourceNode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (OwnerLane)
	{
		OwnerLane->UnregisterResourceNode(this);
	}
	Super::EndPlay(EndPlayReason);
}

void ARTSResourceNode::SnapToLane()
{
	if (!OwnerLane)
	{
		return;
	}
	SplineDistance = FMath::Clamp(SplineDistance, 0.f, OwnerLane->GetSplineLength());
	SetActorLocation(OwnerLane->GetLocationAtDistance(SplineDistance));
}

#if WITH_EDITOR
void ARTSResourceNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	SnapToLane();
}
#endif
