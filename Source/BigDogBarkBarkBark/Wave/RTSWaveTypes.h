// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RTSUnitTypes.h"
#include "RTSWaveTypes.generated.h"

USTRUCT(BlueprintType)
struct FRTSWaveEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Wave")
	ERTSUnitType UnitType = ERTSUnitType::Fox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Wave")
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Wave")
	float SpawnDelay = 0.5f;

	/** -1 = random lane */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Wave")
	int32 PreferredLane = -1;
};

USTRUCT(BlueprintType)
struct FRTSWaveConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Wave")
	TArray<FRTSWaveEntry> Entries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Wave")
	float PreWaveDelay = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Wave")
	bool bIsBossWave = false;
};
