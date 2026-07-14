// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTSWaveTypes.h"
#include "RTSWaveManager.generated.h"

class ARTSLaneSpline;
class ARTSUnitBase;

UCLASS()
class BIGDOGBARKBARKBARK_API ARTSWaveManager : public AActor
{
	GENERATED_BODY()

public:
	ARTSWaveManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Wave")
	TArray<ARTSLaneSpline*> Lanes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Wave")
	TArray<FRTSWaveConfig> Waves;

	/** Fallback if Fox/Wolf/Boss class empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Wave")
	TSubclassOf<ARTSUnitBase> UnitClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Wave")
	TSubclassOf<ARTSUnitBase> FoxUnitClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Wave")
	TSubclassOf<ARTSUnitBase> WolfUnitClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Wave")
	TSubclassOf<ARTSUnitBase> FoxBossUnitClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Wave")
	int32 CurrentWaveIndex = -1;

	UFUNCTION(BlueprintCallable, Category = "RTS|Wave")
	TSubclassOf<ARTSUnitBase> ResolveUnitClass(ERTSUnitType Type) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Wave")
	int32 AliveEnemies = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Wave")
	bool bAllWavesComplete = false;

	UFUNCTION(BlueprintCallable, Category = "RTS|Wave")
	void StartWaves();

	UFUNCTION(BlueprintCallable, Category = "RTS|Wave")
	void NotifyEnemyDied();

	UFUNCTION(BlueprintCallable, Category = "RTS|Wave")
	void NotifyEnemySpawned();

	UFUNCTION(Exec, Category = "RTS|Debug")
	void SkipWave();

	UFUNCTION(BlueprintCallable, Category = "RTS|Wave")
	void SetupDefaultLevel1Waves();

protected:
	virtual void BeginPlay() override;

	void ScheduleNextWave();
	void BeginCurrentWave();
	void SpawnNextInWave();
	void CheckWaveClear();
	ARTSLaneSpline* PickLane(int32 PreferredLane) const;

	FTimerHandle WaveDelayHandle;
	FTimerHandle SpawnDelayHandle;

	int32 EntryIndex = 0;
	int32 SpawnedInEntry = 0;
	bool bWaveActive = false;
	bool bSpawning = false;
};
