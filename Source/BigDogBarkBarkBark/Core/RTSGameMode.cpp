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
#include "RTSGameHUD.h"
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
	HUDWidgetClass = URTSGameHUD::StaticClass();
}

void ARTSGameMode::EnsureCoreClasses()
{
	// BP GameMode / hot-reload can wipe these and cause "Failed to spawn player controller".
	if (!PlayerControllerClass)
	{
		PlayerControllerClass = ARTSPlayerController::StaticClass();
	}
	if (!DefaultPawnClass)
	{
		DefaultPawnClass = ARTSCameraPawn::StaticClass();
	}
	if (!HUDClass)
	{
		HUDClass = ARTSHUD::StaticClass();
	}
}

void ARTSGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	EnsureCoreClasses();
	Fodder = StartingFodder;
	Soul = 0;
	UpgradeLevelRabbit = 0;
	UpgradeLevelChicken = 0;
	UpgradeLevelSheep = 0;
	UpgradeLevelPig = 0;
	bGameOver = false;
	bVictory = false;
	bWavesCleared = false;
}

void ARTSGameMode::BeginPlay()
{
	Super::BeginPlay();
	EnsureCoreClasses();

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
	// Normalize so LaneIndex matches GameMode array index (fixes maps where both lanes are 0).
	for (int32 i = 0; i < Lanes.Num(); ++i)
	{
		if (Lanes[i])
		{
			Lanes[i]->LaneIndex = i;
		}
	}

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
	if (LaneIndex >= 0 && LaneIndex < Lanes.Num() && Lanes[LaneIndex])
	{
		return Lanes[LaneIndex];
	}
	// Also match by authored LaneIndex property (in case array order differs).
	for (ARTSLaneSpline* Lane : Lanes)
	{
		if (Lane && Lane->LaneIndex == LaneIndex)
		{
			return Lane;
		}
	}
	return nullptr;
}

int32 ARTSGameMode::FindLaneArrayIndex(const ARTSLaneSpline* Lane) const
{
	if (!Lane)
	{
		return -1;
	}
	for (int32 i = 0; i < Lanes.Num(); ++i)
	{
		if (Lanes[i] == Lane)
		{
			return i;
		}
	}
	return -1;
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

void ARTSGameMode::AddSoul(int32 Amount)
{
	Soul = FMath::Clamp(Soul + Amount, 0, MaxSoul);
}

bool ARTSGameMode::TrySpendSoul(int32 Amount)
{
	if (Soul < Amount)
	{
		return false;
	}
	Soul -= Amount;
	return true;
}

int32* ARTSGameMode::FindUpgradeLevelMutable(ERTSUnitType Type)
{
	switch (Type)
	{
	case ERTSUnitType::Rabbit: return &UpgradeLevelRabbit;
	case ERTSUnitType::Chicken: return &UpgradeLevelChicken;
	case ERTSUnitType::Sheep: return &UpgradeLevelSheep;
	case ERTSUnitType::Pig: return &UpgradeLevelPig;
	default: return nullptr;
	}
}

const int32* ARTSGameMode::FindUpgradeLevelConst(ERTSUnitType Type) const
{
	switch (Type)
	{
	case ERTSUnitType::Rabbit: return &UpgradeLevelRabbit;
	case ERTSUnitType::Chicken: return &UpgradeLevelChicken;
	case ERTSUnitType::Sheep: return &UpgradeLevelSheep;
	case ERTSUnitType::Pig: return &UpgradeLevelPig;
	default: return nullptr;
	}
}

int32 ARTSGameMode::GetUnitUpgradeLevel(ERTSUnitType Type) const
{
	if (const int32* Found = FindUpgradeLevelConst(Type))
	{
		return *Found;
	}
	return 0;
}

int32 ARTSGameMode::GetUpgradeCost(int32 TargetLevel) const
{
	switch (TargetLevel)
	{
	case 1: return 2;
	case 2: return 4;
	case 3: return 6;
	default: return 0;
	}
}

int32 ARTSGameMode::GetEffectiveFodderCost(ERTSUnitType Type) const
{
	const FRTSUnitStats Base = RTSUnitData::GetDefaultStats(Type);
	if (GetUnitUpgradeLevel(Type) >= 3)
	{
		return FMath::Max(1, FMath::RoundToInt(Base.FodderCost * 0.8f));
	}
	return Base.FodderCost;
}

void ARTSGameMode::ApplyUpgradeToStats(FRTSUnitStats& Stats, int32 Level) const
{
	if (Level >= 1)
	{
		Stats.MaxHealth *= 1.5f;
	}
	if (Level >= 2)
	{
		Stats.AttackPower *= 1.5f;
	}
	if (Level >= 3)
	{
		Stats.FodderCost = FMath::Max(1, FMath::RoundToInt(Stats.FodderCost * 0.8f));
	}
}

bool ARTSGameMode::TryUpgradeUnitType(ERTSUnitType Type)
{
	if (!RTSUnitData::IsFarmRecruitable(Type))
	{
		return false;
	}

	int32* LevelPtr = FindUpgradeLevelMutable(Type);
	if (!LevelPtr)
	{
		return false;
	}

	const int32 Current = *LevelPtr;
	if (Current >= 3)
	{
		return false;
	}

	const int32 NextLevel = Current + 1;
	const int32 Cost = GetUpgradeCost(NextLevel);
	if (!TrySpendSoul(Cost))
	{
		return false;
	}

	*LevelPtr = NextLevel;
	return true;
}

int32 ARTSGameMode::GetSoulDropForUnit(const ARTSUnitBase* Unit) const
{
	if (!Unit)
	{
		return 0;
	}
	return Unit->Stats.bIsBoss ? 5 : 1;
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

	const int32 Cost = GetEffectiveFodderCost(Type);
	if (!TrySpendFodder(Cost))
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
		AddFodder(Cost);
		return nullptr;
	}

	Unit->InitializeUnit(Type, Lane, 0.f, Mode);

	const int32 Level = GetUnitUpgradeLevel(Type);
	if (Level > 0)
	{
		ApplyUpgradeToStats(Unit->Stats, Level);
		Unit->CurrentHealth = Unit->Stats.MaxHealth;
	}

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
		AddSoul(GetSoulDropForUnit(Unit));
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
	UGameplayStatics::SetGamePaused(this, true);

	if (ARTSPlayerController* PC = Cast<ARTSPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		if (PC->HUDWidget)
		{
			PC->HUDWidget->ShowDefeatScreen();
		}
	}
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

void ARTSGameMode::AddSoulCmd(int32 Amount)
{
	AddSoul(Amount);
}

void ARTSGameMode::UpgradeUnitCmd(int32 UnitTypeIndex)
{
	ERTSUnitType Type = ERTSUnitType::None;
	switch (UnitTypeIndex)
	{
	case 1: Type = ERTSUnitType::Rabbit; break;
	case 2: Type = ERTSUnitType::Chicken; break;
	case 3: Type = ERTSUnitType::Sheep; break;
	case 4: Type = ERTSUnitType::Pig; break;
	default: return;
	}
	TryUpgradeUnitType(Type);
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
