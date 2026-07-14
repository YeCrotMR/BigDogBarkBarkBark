// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSWaveManager.h"
#include "RTSLaneSpline.h"
#include "RTSUnitBase.h"
#include "RTSGameMode.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

ARTSWaveManager::ARTSWaveManager()
{
	PrimaryActorTick.bCanEverTick = false;
	UnitClass = ARTSUnitBase::StaticClass();
}

TSubclassOf<ARTSUnitBase> ARTSWaveManager::ResolveUnitClass(ERTSUnitType Type) const
{
	TSubclassOf<ARTSUnitBase> Specific = nullptr;
	switch (Type)
	{
	case ERTSUnitType::Fox:		Specific = FoxUnitClass; break;
	case ERTSUnitType::Wolf:	Specific = WolfUnitClass; break;
	case ERTSUnitType::FoxBoss:	Specific = FoxBossUnitClass; break;
	default: break;
	}
	return Specific ? Specific : UnitClass;
}

void ARTSWaveManager::BeginPlay()
{
	Super::BeginPlay();
	if (Waves.Num() == 0)
	{
		SetupDefaultLevel1Waves();
	}
}

static FRTSWaveEntry MakeEntry(ERTSUnitType Type, int32 Count, float Delay, int32 Lane)
{
	FRTSWaveEntry Entry;
	Entry.UnitType = Type;
	Entry.Count = Count;
	Entry.SpawnDelay = Delay;
	Entry.PreferredLane = Lane;
	return Entry;
}

void ARTSWaveManager::SetupDefaultLevel1Waves()
{
	Waves.Reset();

	FRTSWaveConfig Wave1;
	Wave1.PreWaveDelay = 10.f;
	Wave1.Entries.Add(MakeEntry(ERTSUnitType::Fox, 2, 0.6f, -1));
	Waves.Add(Wave1);

	FRTSWaveConfig Wave2;
	Wave2.PreWaveDelay = 25.f;
	Wave2.Entries.Add(MakeEntry(ERTSUnitType::Wolf, 2, 0.6f, -1));
	Waves.Add(Wave2);

	FRTSWaveConfig Wave3;
	Wave3.PreWaveDelay = 30.f;
	Wave3.Entries.Add(MakeEntry(ERTSUnitType::Wolf, 2, 0.6f, -1));
	Wave3.Entries.Add(MakeEntry(ERTSUnitType::Fox, 2, 0.6f, -1));
	Waves.Add(Wave3);

	FRTSWaveConfig Wave4;
	Wave4.PreWaveDelay = 30.f;
	Wave4.bIsBossWave = true;
	Wave4.Entries.Add(MakeEntry(ERTSUnitType::FoxBoss, 1, 0.5f, -1));
	Wave4.Entries.Add(MakeEntry(ERTSUnitType::Fox, 2, 0.6f, -1));
	Waves.Add(Wave4);
}

void ARTSWaveManager::StartWaves()
{
	if (Lanes.Num() == 0)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARTSLaneSpline::StaticClass(), Found);
		for (AActor* Actor : Found)
		{
			Lanes.Add(Cast<ARTSLaneSpline>(Actor));
		}
		Lanes.Sort([](const ARTSLaneSpline& A, const ARTSLaneSpline& B)
		{
			return A.LaneIndex < B.LaneIndex;
		});
	}

	CurrentWaveIndex = -1;
	AliveEnemies = 0;
	bAllWavesComplete = false;
	ScheduleNextWave();
}

void ARTSWaveManager::ScheduleNextWave()
{
	++CurrentWaveIndex;
	if (CurrentWaveIndex >= Waves.Num())
	{
		bAllWavesComplete = true;
		if (AliveEnemies <= 0)
		{
			if (ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this)))
			{
				GM->NotifyAllWavesCleared();
			}
		}
		return;
	}

	bWaveActive = false;
	bSpawning = false;
	const float Delay = Waves[CurrentWaveIndex].PreWaveDelay;
	GetWorldTimerManager().SetTimer(WaveDelayHandle, this, &ARTSWaveManager::BeginCurrentWave, Delay, false);
}

void ARTSWaveManager::BeginCurrentWave()
{
	bWaveActive = true;
	bSpawning = true;
	EntryIndex = 0;
	SpawnedInEntry = 0;
	SpawnNextInWave();
}

void ARTSWaveManager::SpawnNextInWave()
{
	if (!bSpawning || CurrentWaveIndex < 0 || CurrentWaveIndex >= Waves.Num())
	{
		return;
	}

	FRTSWaveConfig& Wave = Waves[CurrentWaveIndex];
	if (EntryIndex >= Wave.Entries.Num())
	{
		bSpawning = false;
		CheckWaveClear();
		return;
	}

	FRTSWaveEntry& Entry = Wave.Entries[EntryIndex];
	ARTSLaneSpline* Lane = PickLane(Entry.PreferredLane);
	TSubclassOf<ARTSUnitBase> SpawnClass = ResolveUnitClass(Entry.UnitType);
	if (!Lane || !SpawnClass)
	{
		return;
	}

	const float SpawnDist = Lane->GetSplineLength();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ARTSUnitBase* Unit = GetWorld()->SpawnActor<ARTSUnitBase>(SpawnClass, Lane->GetLocationAtDistance(SpawnDist), FRotator::ZeroRotator, Params);
	if (Unit)
	{
		Unit->InitializeUnit(Entry.UnitType, Lane, SpawnDist, EUnitWorkMode::Combat);
		if (ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GM->NotifyUnitSpawned(Unit);
		}
		else
		{
			NotifyEnemySpawned();
		}
	}

	++SpawnedInEntry;
	if (SpawnedInEntry >= Entry.Count)
	{
		SpawnedInEntry = 0;
		++EntryIndex;
	}

	const float NextDelay = Entry.SpawnDelay;
	GetWorldTimerManager().SetTimer(SpawnDelayHandle, this, &ARTSWaveManager::SpawnNextInWave, NextDelay, false);
}

ARTSLaneSpline* ARTSWaveManager::PickLane(int32 PreferredLane) const
{
	if (Lanes.Num() == 0)
	{
		return nullptr;
	}
	if (PreferredLane >= 0 && PreferredLane < Lanes.Num() && Lanes[PreferredLane])
	{
		return Lanes[PreferredLane];
	}
	const int32 Index = FMath::RandRange(0, Lanes.Num() - 1);
	return Lanes[Index];
}

void ARTSWaveManager::NotifyEnemySpawned()
{
	++AliveEnemies;
}

void ARTSWaveManager::NotifyEnemyDied()
{
	AliveEnemies = FMath::Max(0, AliveEnemies - 1);
	CheckWaveClear();
}

void ARTSWaveManager::CheckWaveClear()
{
	if (bSpawning || AliveEnemies > 0)
	{
		return;
	}

	if (bAllWavesComplete)
	{
		if (ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GM->NotifyAllWavesCleared();
		}
		return;
	}

	if (bWaveActive)
	{
		bWaveActive = false;
		ScheduleNextWave();
	}
}

void ARTSWaveManager::SkipWave()
{
	GetWorldTimerManager().ClearTimer(WaveDelayHandle);
	GetWorldTimerManager().ClearTimer(SpawnDelayHandle);
	bSpawning = false;
	bWaveActive = false;
	ScheduleNextWave();
}
