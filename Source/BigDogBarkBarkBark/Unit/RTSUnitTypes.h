// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RTSUnitTypes.generated.h"

UENUM(BlueprintType)
enum class ERTSUnitType : uint8
{
	None		UMETA(DisplayName = "None"),
	Rabbit		UMETA(DisplayName = "Rabbit"),
	Chicken		UMETA(DisplayName = "Chicken"),
	Sheep		UMETA(DisplayName = "Sheep"),
	Pig			UMETA(DisplayName = "Pig"),
	Fox			UMETA(DisplayName = "Fox"),
	Wolf		UMETA(DisplayName = "Wolf"),
	FoxBoss		UMETA(DisplayName = "Fox Boss")
};

UENUM(BlueprintType)
enum class EUnitWorkMode : uint8
{
	Combat		UMETA(DisplayName = "Combat"),
	Collect		UMETA(DisplayName = "Collect")
};

UENUM(BlueprintType)
enum class ERTSUnitState : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	Moving		UMETA(DisplayName = "Moving"),
	Combat		UMETA(DisplayName = "Combat"),
	Collecting	UMETA(DisplayName = "Collecting"),
	Guarding	UMETA(DisplayName = "Guarding"),
	Dead		UMETA(DisplayName = "Dead")
};

UENUM(BlueprintType)
enum class ERTSTeam : uint8
{
	Farm		UMETA(DisplayName = "Farm"),
	Wild		UMETA(DisplayName = "Wild")
};

USTRUCT(BlueprintType)
struct FRTSUnitStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	ERTSUnitType UnitType = ERTSUnitType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	int32 FodderCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	float MaxHealth = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	float AttackPower = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	float MoveSpeed = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	float CollectEfficiencyMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	bool bIsBoss = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	bool bAmbushTrait = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	bool bPackRushTrait = false;
};

namespace RTSUnitData
{
	inline FRTSUnitStats GetDefaultStats(ERTSUnitType Type)
	{
		FRTSUnitStats Stats;
		Stats.UnitType = Type;

		switch (Type)
		{
		case ERTSUnitType::Rabbit:
			Stats.FodderCost = 12;
			Stats.MaxHealth = 28.f;
			Stats.AttackPower = 9.f;
			Stats.CollectEfficiencyMultiplier = 2.f;
			break;
		case ERTSUnitType::Chicken:
			Stats.FodderCost = 12;
			Stats.MaxHealth = 22.f;
			Stats.AttackPower = 7.f;
			Stats.CollectEfficiencyMultiplier = 2.f;
			break;
		case ERTSUnitType::Sheep:
			Stats.FodderCost = 20;
			Stats.MaxHealth = 52.f;
			Stats.AttackPower = 12.f;
			break;
		case ERTSUnitType::Pig:
			Stats.FodderCost = 30;
			Stats.MaxHealth = 65.f;
			Stats.AttackPower = 15.f;
			break;
		case ERTSUnitType::Fox:
			Stats.MaxHealth = 42.f;
			Stats.AttackPower = 15.f;
			Stats.bAmbushTrait = true;
			break;
		case ERTSUnitType::Wolf:
			Stats.MaxHealth = 58.f;
			Stats.AttackPower = 19.f;
			Stats.bPackRushTrait = true;
			break;
		case ERTSUnitType::FoxBoss:
			Stats.MaxHealth = 160.f;
			Stats.AttackPower = 25.f;
			Stats.bIsBoss = true;
			break;
		default:
			break;
		}

		return Stats;
	}

	inline ERTSTeam GetTeam(ERTSUnitType Type)
	{
		switch (Type)
		{
		case ERTSUnitType::Fox:
		case ERTSUnitType::Wolf:
		case ERTSUnitType::FoxBoss:
			return ERTSTeam::Wild;
		default:
			return ERTSTeam::Farm;
		}
	}

	inline bool IsFarmRecruitable(ERTSUnitType Type)
	{
		return Type == ERTSUnitType::Rabbit
			|| Type == ERTSUnitType::Chicken
			|| Type == ERTSUnitType::Sheep
			|| Type == ERTSUnitType::Pig;
	}
}
