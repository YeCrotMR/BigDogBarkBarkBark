// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTSLevelBootstrap.generated.h"

/**
 * Spawns a default Level-1 layout (2 lanes, 1 resource node, coop, wave manager)
 * when the map has no ARTSLaneSpline actors. Place manually or let GameMode spawn one.
 */
UCLASS()
class BIGDOGBARKBARKBARK_API ARTSLevelBootstrap : public AActor
{
	GENERATED_BODY()

public:
	ARTSLevelBootstrap();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Bootstrap")
	bool bAutoSpawnIfMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Bootstrap")
	float LaneLength = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Bootstrap")
	float LaneSpacing = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Bootstrap")
	float ResourceDistance = 900.f;

	UFUNCTION(BlueprintCallable, Category = "RTS|Bootstrap")
	void EnsureLevel1Layout();

protected:
	virtual void BeginPlay() override;
};
