// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RTSUnitTypes.h"
#include "RTSGameMode.generated.h"

class ARTSUnitBase;
class ARTSLaneSpline;
class ARTSWaveManager;
class ARTSBaseBuilding;
class URTSGameHUD;

UCLASS()
class BIGDOGBARKBARKBARK_API ARTSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARTSGameMode();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Economy")
	int32 StartingFodder = 50;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Economy")
	int32 Fodder = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Economy")
	int32 MaxFodder = 999;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Economy")
	float KillFodderDropChance = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Economy")
	int32 KillFodderDropAmount = 5;

	/** Fallback if a per-type class below is empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Spawn")
	TSubclassOf<ARTSUnitBase> FarmUnitClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Spawn")
	TSubclassOf<ARTSUnitBase> RabbitUnitClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Spawn")
	TSubclassOf<ARTSUnitBase> ChickenUnitClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Spawn")
	TSubclassOf<ARTSUnitBase> SheepUnitClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Spawn")
	TSubclassOf<ARTSUnitBase> PigUnitClass;

	/** Fallback wild unit if Fox/Wolf/Boss class empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Spawn")
	TSubclassOf<ARTSUnitBase> WildUnitClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Spawn")
	TSubclassOf<ARTSUnitBase> FoxUnitClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Spawn")
	TSubclassOf<ARTSUnitBase> WolfUnitClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Spawn")
	TSubclassOf<ARTSUnitBase> FoxBossUnitClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|UI")
	TSubclassOf<URTSGameHUD> HUDWidgetClass;

	/** Picks Rabbit/Chicken/... class, else FarmUnitClass. */
	UFUNCTION(BlueprintCallable, Category = "RTS|Spawn")
	TSubclassOf<ARTSUnitBase> ResolveFarmUnitClass(ERTSUnitType Type) const;

	/** Picks Fox/Wolf/FoxBoss class, else WildUnitClass. */
	UFUNCTION(BlueprintCallable, Category = "RTS|Spawn")
	TSubclassOf<ARTSUnitBase> ResolveWildUnitClass(ERTSUnitType Type) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|State")
	bool bGameOver = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|State")
	bool bVictory = false;

	UFUNCTION(BlueprintCallable, Category = "RTS|Economy")
	void AddFodder(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "RTS|Economy")
	bool TrySpendFodder(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "RTS|Recruit")
	ARTSUnitBase* RecruitUnit(ERTSUnitType Type, int32 LaneIndex, EUnitWorkMode Mode = EUnitWorkMode::Combat);

	UFUNCTION(BlueprintCallable, Category = "RTS|Lane")
	ARTSLaneSpline* GetLaneByIndex(int32 LaneIndex) const;

	UFUNCTION(BlueprintCallable, Category = "RTS|Events")
	void NotifyUnitSpawned(ARTSUnitBase* Unit);

	UFUNCTION(BlueprintCallable, Category = "RTS|Events")
	void NotifyUnitDied(ARTSUnitBase* Unit);

	UFUNCTION(BlueprintCallable, Category = "RTS|Events")
	void NotifyCoreDestroyed();

	UFUNCTION(BlueprintCallable, Category = "RTS|Events")
	void NotifyAllWavesCleared();

	UFUNCTION(Exec, Category = "RTS|Debug")
	void AddFodderCmd(int32 Amount = 50);

	UFUNCTION(Exec, Category = "RTS|Debug")
	void SkipWave();

	UFUNCTION(BlueprintCallable, Category = "RTS|State")
	FText GetStatusText() const;

	UFUNCTION(BlueprintCallable, Category = "RTS|Wave")
	ARTSWaveManager* GetWaveManager() const { return WaveManager; }

	/** Public for HUD display. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Wave")
	ARTSWaveManager* WaveManager = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	void CacheLevelActors();
	void TryVictory();

	UPROPERTY()
	TArray<ARTSLaneSpline*> Lanes;

	UPROPERTY()
	ARTSBaseBuilding* CoreBuilding = nullptr;

	bool bWavesCleared = false;
};
