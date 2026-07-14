// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSGameMode.h"
#include "RTSPlayerController.h"
#include "RTSCameraPawn.h"
#include "RTSLaneSpline.h"
#include "RTSResourceNode.h"
#include "RTSUnitBase.h"
#include "RTSBaseBuilding.h"
#include "RTSWaveManager.h"
#include "RTSHUD.h"
#include "RTSLevelBootstrap.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

ARTSGameMode::ARTSGameMode()
{
	DefaultPawnClass = ARTSCameraPawn::StaticClass();
	PlayerControllerClass = ARTSPlayerController::StaticClass();
	HUDClass = ARTSHUD::StaticClass();
	FarmUnitClass = ARTSUnitBase::StaticClass();
}

void ARTSGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	Fodder = StartingFodder;
	bGameOver = false;
	bVictory = false;
	bWavesCleared = false;
}

void ARTSGameMode::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> ExistingLanes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARTSLaneSpline::StaticClass(), ExistingLanes);
	if (ExistingLanes.Num() == 0)
	{
		FVector Origin = FVector::ZeroVector;
		if (AActor* PlayerStart = UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()))
		{
			Origin = PlayerStart->GetActorLocation();
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ARTSLevelBootstrap* Bootstrap = GetWorld()->SpawnActor<ARTSLevelBootstrap>(Origin, FRotator::ZeroRotator, Params);
		if (Bootstrap)
		{
			Bootstrap->EnsureLevel1Layout();
		}
	}

	CacheLevelActors();

	if (WaveManager)
	{
		WaveManager->StartWaves();
	}
}

void ARTSGameMode::CacheLevelActors()
{
	Lanes.Reset();
	TArray<AActor*> FoundLanes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARTSLaneSpline::StaticClass(), FoundLanes);
	for (AActor* Actor : FoundLanes)
	{
		Lanes.Add(Cast<ARTSLaneSpline>(Actor));
	}
	Lanes.Sort([](const ARTSLaneSpline& A, const ARTSLaneSpline& B)
	{
		return A.LaneIndex < B.LaneIndex;
	});

	TArray<AActor*> FoundWaves;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARTSWaveManager::StaticClass(), FoundWaves);
	WaveManager = FoundWaves.Num() > 0 ? Cast<ARTSWaveManager>(FoundWaves[0]) : nullptr;

	if (!WaveManager)
	{
		FActorSpawnParameters Params;
		WaveManager = GetWorld()->SpawnActor<ARTSWaveManager>(ARTSWaveManager::StaticClass(), Params);
	}
	if (WaveManager)
	{
		WaveManager->Lanes = Lanes;
		if (WildUnitClass)
		{
			WaveManager->UnitClass = WildUnitClass;
		}
		WaveManager->FoxUnitClass = FoxUnitClass;
		WaveManager->WolfUnitClass = WolfUnitClass;
		WaveManager->FoxBossUnitClass = FoxBossUnitClass;
	}

	TArray<AActor*> FoundBuildings;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARTSBaseBuilding::StaticClass(), FoundBuildings);
	for (AActor* Actor : FoundBuildings)
	{
		ARTSBaseBuilding* Building = Cast<ARTSBaseBuilding>(Actor);
		if (Building && Building->bIsCoreBuilding)
		{
			CoreBuilding = Building;
			break;
		}
	}
}

ARTSLaneSpline* ARTSGameMode::GetLaneByIndex(int32 LaneIndex) const
{
	if (LaneIndex >= 0 && LaneIndex < Lanes.Num())
	{
		return Lanes[LaneIndex];
	}
	return nullptr;
}

void ARTSGameMode::AddFodder(int32 Amount)
{
	Fodder = FMath::Clamp(Fodder + Amount, 0, MaxFodder);
}

bool ARTSGameMode::TrySpendFodder(int32 Amount)
{
	if (Fodder < Amount)
	{
		return false;
	}
	Fodder -= Amount;
	return true;
}

TSubclassOf<ARTSUnitBase> ARTSGameMode::ResolveFarmUnitClass(ERTSUnitType Type) const
{
	TSubclassOf<ARTSUnitBase> Specific = nullptr;
	switch (Type)
	{
	case ERTSUnitType::Rabbit:	Specific = RabbitUnitClass; break;
	case ERTSUnitType::Chicken:	Specific = ChickenUnitClass; break;
	case ERTSUnitType::Sheep:	Specific = SheepUnitClass; break;
	case ERTSUnitType::Pig:		Specific = PigUnitClass; break;
	default: break;
	}
	return Specific ? Specific : FarmUnitClass;
}

TSubclassOf<ARTSUnitBase> ARTSGameMode::ResolveWildUnitClass(ERTSUnitType Type) const
{
	TSubclassOf<ARTSUnitBase> Specific = nullptr;
	switch (Type)
	{
	case ERTSUnitType::Fox:		Specific = FoxUnitClass; break;
	case ERTSUnitType::Wolf:	Specific = WolfUnitClass; break;
	case ERTSUnitType::FoxBoss:	Specific = FoxBossUnitClass; break;
	default: break;
	}
	if (Specific)
	{
		return Specific;
	}
	if (WildUnitClass)
	{
		return WildUnitClass;
	}
	// Last resort: WaveManager default class
	if (WaveManager && WaveManager->UnitClass)
	{
		return WaveManager->UnitClass;
	}
	return ARTSUnitBase::StaticClass();
}

ARTSUnitBase* ARTSGameMode::RecruitUnit(ERTSUnitType Type, int32 LaneIndex, EUnitWorkMode Mode)
{
	TSubclassOf<ARTSUnitBase> SpawnClass = ResolveFarmUnitClass(Type);
	if (bGameOver || !RTSUnitData::IsFarmRecruitable(Type) || !SpawnClass)
	{
		return nullptr;
	}

	ARTSLaneSpline* Lane = GetLaneByIndex(LaneIndex);
	if (!Lane && Lanes.Num() > 0)
	{
		Lane = Lanes[0];
	}
	if (!Lane)
	{
		return nullptr;
	}

	const FRTSUnitStats Stats = RTSUnitData::GetDefaultStats(Type);
	if (!TrySpendFodder(Stats.FodderCost))
	{
		return nullptr;
	}

	if (Mode == EUnitWorkMode::Collect && !Lane->FindNearestResourceNode(0.f))
	{
		for (ARTSLaneSpline* Candidate : Lanes)
		{
			if (Candidate && Candidate->FindNearestResourceNode(0.f))
			{
				Lane = Candidate;
				break;
			}
		}
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector SpawnLoc = Lane->GetLocationAtDistance(0.f);
	ARTSUnitBase* Unit = GetWorld()->SpawnActor<ARTSUnitBase>(SpawnClass, SpawnLoc, FRotator::ZeroRotator, Params);
	if (!Unit)
	{
		AddFodder(Stats.FodderCost);
		return nullptr;
	}

	Unit->InitializeUnit(Type, Lane, 0.f, Mode);
	NotifyUnitSpawned(Unit);
	return Unit;
}

void ARTSGameMode::NotifyUnitSpawned(ARTSUnitBase* Unit)
{
	if (Unit && Unit->Team == ERTSTeam::Wild && WaveManager)
	{
		WaveManager->NotifyEnemySpawned();
	}
}

void ARTSGameMode::NotifyUnitDied(ARTSUnitBase* Unit)
{
	if (!Unit || bGameOver)
	{
		return;
	}

	if (Unit->Team == ERTSTeam::Wild)
	{
		if (FMath::FRand() <= KillFodderDropChance)
		{
			AddFodder(KillFodderDropAmount);
		}
		if (WaveManager)
		{
			WaveManager->NotifyEnemyDied();
		}
		TryVictory();
	}
}

void ARTSGameMode::NotifyCoreDestroyed()
{
	if (bGameOver)
	{
		return;
	}
	bGameOver = true;
	bVictory = false;
}

void ARTSGameMode::NotifyAllWavesCleared()
{
	bWavesCleared = true;
	TryVictory();
}

void ARTSGameMode::TryVictory()
{
	if (bGameOver)
	{
		return;
	}
	if (bWavesCleared && WaveManager && WaveManager->AliveEnemies <= 0 && CoreBuilding && CoreBuilding->IsAlive())
	{
		bGameOver = true;
		bVictory = true;
	}
}

void ARTSGameMode::AddFodderCmd(int32 Amount)
{
	AddFodder(Amount);
}

void ARTSGameMode::SkipWave()
{
	if (WaveManager)
	{
		WaveManager->SkipWave();
	}
}

FText ARTSGameMode::GetStatusText() const
{
	if (bGameOver)
	{
		return bVictory
			? FText::FromString(TEXT("Victory - Fox-Wolf Coalition Repelled!"))
			: FText::FromString(TEXT("Defeat - The Coop Was Destroyed"));
	}
	return FText::GetEmpty();
}
