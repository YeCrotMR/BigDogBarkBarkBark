// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSHUD.h"
#include "RTSGameMode.h"
#include "RTSPlayerController.h"
#include "RTSUnitBase.h"
#include "RTSWaveManager.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"

void ARTSHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;
	if (!Font)
	{
		Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	}

	ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayerController());

	float Y = 40.f;
	const float X = 40.f;
	const float Line = 28.f;

	auto DrawLine = [&](const FString& Text, const FLinearColor& Color = FLinearColor::White)
	{
		FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), Font, Color);
		Item.EnableShadow(FLinearColor::Black);
		Item.Scale = FVector2D(1.25f, 1.25f);
		Canvas->DrawItem(Item);
		Y += Line;
	};

	DrawLine(TEXT("=== BigDog RTS Level 1 ==="), FLinearColor::Yellow);

	if (GM)
	{
		DrawLine(FString::Printf(TEXT("Fodder: %d"), GM->Fodder), FLinearColor::Green);
		if (ARTSWaveManager* WM = GM->GetWaveManager())
		{
			DrawLine(FString::Printf(TEXT("Wave: %d / Alive enemies: %d"),
				WM->CurrentWaveIndex + 1,
				WM->AliveEnemies));
		}
		const FText Status = GM->GetStatusText();
		if (!Status.IsEmpty())
		{
			DrawLine(Status.ToString(), GM->bVictory ? FLinearColor::Green : FLinearColor::Red);
		}
	}
	else
	{
		DrawLine(TEXT("ERROR: GameMode is NOT RTSGameMode"), FLinearColor::Red);
		DrawLine(TEXT("World Settings -> GameMode Override -> RTSGameMode"), FLinearColor::Red);
	}

	if (PC)
	{
		FString Mode = TEXT("-");
		if (PC->SelectedUnit && PC->SelectedUnit->IsAlive())
		{
			Mode = PC->SelectedUnit->WorkMode == EUnitWorkMode::Collect ? TEXT("Collect") : TEXT("Combat");
		}
		DrawLine(FString::Printf(TEXT("Lane: %d | Unit mode: %s"), PC->SelectedLaneIndex, *Mode));
	}

	Y += 10.f;
	DrawLine(TEXT("1 Rabbit | 2 Chicken | 3 Sheep | 4 Pig"), FLinearColor(0.8f, 0.9f, 1.f));
	DrawLine(TEXT("Q/E Lane | T Mode | A/D Camera | Wheel Zoom"), FLinearColor(0.8f, 0.9f, 1.f));
	DrawLine(TEXT("Console: AddFodderCmd 100 | SkipWave"), FLinearColor(0.7f, 0.7f, 0.7f));
}
