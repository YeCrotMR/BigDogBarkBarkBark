// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSLevelBootstrap.h"
#include "RTSLaneSpline.h"
#include "RTSResourceNode.h"
#include "RTSBaseBuilding.h"
#include "RTSWaveManager.h"
#include "Components/SplineComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

ARTSLevelBootstrap::ARTSLevelBootstrap()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARTSLevelBootstrap::BeginPlay()
{
	Super::BeginPlay();
	if (bAutoSpawnIfMissing)
	{
		EnsureLevel1Layout();
	}
}

void ARTSLevelBootstrap::EnsureLevel1Layout()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> ExistingLanes;
	UGameplayStatics::GetAllActorsOfClass(World, ARTSLaneSpline::StaticClass(), ExistingLanes);
	if (ExistingLanes.Num() > 0)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Origin = GetActorLocation();

	TArray<ARTSLaneSpline*> CreatedLanes;
	for (int32 i = 0; i < 2; ++i)
	{
		// Space lanes along X; run each lane forward along +Y (into camera view).
		const FVector LaneStart = Origin + FVector((i - 0.5f) * LaneSpacing, 0.f, 0.f);
		ARTSLaneSpline* Lane = World->SpawnActor<ARTSLaneSpline>(LaneStart, FRotator::ZeroRotator, Params);
		if (!Lane || !Lane->Spline)
		{
			continue;
		}
		Lane->LaneIndex = i;
		Lane->Spline->ClearSplinePoints(false);
		Lane->Spline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local, false);
		Lane->Spline->AddSplinePoint(FVector(0.f, LaneLength, 0.f), ESplineCoordinateSpace::Local, true);
		Lane->Spline->UpdateSpline();
		Lane->SetActorLocation(LaneStart);
		CreatedLanes.Add(Lane);
	}

	if (CreatedLanes.Num() > 0)
	{
		ARTSResourceNode* Node = World->SpawnActor<ARTSResourceNode>(
			CreatedLanes[0]->GetLocationAtDistance(ResourceDistance), FRotator::ZeroRotator, Params);
		if (Node)
		{
			Node->OwnerLane = CreatedLanes[0];
			Node->SplineDistance = ResourceDistance;
			Node->SnapToLane();
			CreatedLanes[0]->RegisterResourceNode(Node);
		}
	}

	const FVector CoopLoc = Origin + FVector(0.f, -200.f, 0.f);
	ARTSBaseBuilding* Coop = World->SpawnActor<ARTSBaseBuilding>(CoopLoc, FRotator::ZeroRotator, Params);
	if (Coop)
	{
		Coop->bIsCoreBuilding = true;
		Coop->MaxHealth = 500.f;
		Coop->CurrentHealth = 500.f;
	}

	TArray<AActor*> ExistingWaves;
	UGameplayStatics::GetAllActorsOfClass(World, ARTSWaveManager::StaticClass(), ExistingWaves);
	if (ExistingWaves.Num() == 0)
	{
		ARTSWaveManager* WM = World->SpawnActor<ARTSWaveManager>(Origin, FRotator::ZeroRotator, Params);
		if (WM)
		{
			WM->Lanes = CreatedLanes;
			WM->SetupDefaultLevel1Waves();
		}
	}
}
