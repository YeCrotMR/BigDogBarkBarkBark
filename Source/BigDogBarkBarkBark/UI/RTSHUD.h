// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RTSHUD.generated.h"

UCLASS()
class BIGDOGBARKBARKBARK_API ARTSHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	void DrawProjectedHealth(APlayerController* PC, const FVector& WorldLoc, float Current, float Max, const FLinearColor& FillColor);
};
