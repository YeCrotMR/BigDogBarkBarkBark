// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSHUD.h"
#include "RTSGameMode.h"
#include "RTSPlayerController.h"
#include "RTSUnitBase.h"
#include "RTSBaseBuilding.h"
#include "RTSWaveManager.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"

void ARTSHUD::DrawProjectedHealth(APlayerController* PC, const FVector& WorldLoc, float Current, float Max, const FLinearColor& FillColor)
{
	if (!Canvas || !PC || Max <= 0.f)
	{
		return;
	}

	FVector2D ScreenPos;
	if (!PC->ProjectWorldLocationToScreen(WorldLoc, ScreenPos, true))
	{
		return;
	}

	constexpr float Margin = 50.f;
	if (ScreenPos.X < -Margin || ScreenPos.Y < -Margin
		|| ScreenPos.X > Canvas->SizeX + Margin || ScreenPos.Y > Canvas->SizeY + Margin)
	{
		return;
	}

	const float Percent = FMath::Clamp(Current / Max, 0.f, 1.f);
	constexpr float BarW = 70.f;
	constexpr float BarH = 8.f;
	const float BarX = ScreenPos.X - BarW * 0.5f;
	const float BarY = ScreenPos.Y - 28.f;

	FCanvasTileItem Bg(FVector2D(BarX, BarY), FVector2D(BarW, BarH), FLinearColor(0.08f, 0.08f, 0.08f, 0.85f));
	Bg.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Bg);

	if (Percent > 0.f)
	{
		FCanvasTileItem Fill(FVector2D(BarX, BarY), FVector2D(BarW * Percent, BarH), FillColor);
		Fill.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Fill);
	}

	UFont* SmallFont = GEngine ? GEngine->GetSmallFont() : nullptr;
	if (!SmallFont)
	{
		SmallFont = GEngine ? GEngine->GetMediumFont() : nullptr;
	}

	const FString Label = FString::Printf(TEXT("%d/%d"), FMath::RoundToInt(Current), FMath::RoundToInt(Max));
	FCanvasTextItem Text(FVector2D(ScreenPos.X, BarY - 14.f), FText::FromString(Label), SmallFont, FLinearColor::White);
	Text.bCentreX = true;
	Text.EnableShadow(FLinearColor::Black);
	Text.Scale = FVector2D(0.85f, 0.85f);
	Canvas->DrawItem(Text);
}

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

	// World-space health bars for all units and buildings
	if (PC)
	{
		TArray<AActor*> Units;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARTSUnitBase::StaticClass(), Units);
		for (AActor* Actor : Units)
		{
			ARTSUnitBase* Unit = Cast<ARTSUnitBase>(Actor);
			if (!Unit || !Unit->IsAlive())
			{
				continue;
			}

			FLinearColor FillColor(0.95f, 0.15f, 0.15f, 1.f);
			if (Unit->Team == ERTSTeam::Farm)
			{
				FillColor = (Unit->WorkMode == EUnitWorkMode::Collect)
					? FLinearColor(1.f, 0.88f, 0.12f, 1.f)
					: FLinearColor(0.2f, 0.55f, 1.f, 1.f);
			}

			const FVector WorldLoc = Unit->GetActorLocation() + FVector(0.f, 0.f, 120.f);
			DrawProjectedHealth(PC, WorldLoc, Unit->CurrentHealth, Unit->Stats.MaxHealth, FillColor);
		}

		TArray<AActor*> Buildings;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARTSBaseBuilding::StaticClass(), Buildings);
		for (AActor* Actor : Buildings)
		{
			ARTSBaseBuilding* Building = Cast<ARTSBaseBuilding>(Actor);
			if (!Building || !Building->IsAlive())
			{
				continue;
			}

			const FVector WorldLoc = Building->GetActorLocation() + FVector(0.f, 0.f, 200.f);
			DrawProjectedHealth(PC, WorldLoc, Building->CurrentHealth, Building->MaxHealth,
				FLinearColor(0.82f, 0.82f, 0.82f, 1.f));
		}
	}

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
