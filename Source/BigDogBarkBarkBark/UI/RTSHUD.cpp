// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSHUD.h"
#include "RTSGameMode.h"
#include "RTSPlayerController.h"
#include "RTSGameHUD.h"
#include "RTSUnitBase.h"
#include "RTSBaseBuilding.h"
#include "RTSResourceNode.h"
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

void ARTSHUD::DrawProjectedLabel(APlayerController* PC, const FVector& WorldLoc, const FString& Text, const FLinearColor& Color)
{
	if (!Canvas || !PC || Text.IsEmpty())
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

	UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;
	if (!Font)
	{
		Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	}
	if (!Font)
	{
		return;
	}

	FCanvasTextItem Item(FVector2D(ScreenPos.X, ScreenPos.Y), FText::FromString(Text), Font, Color);
	Item.bCentreX = true;
	Item.bCentreY = true;
	Item.EnableShadow(FLinearColor::Black);
	Item.Scale = FVector2D(1.7f, 1.7f);
	Canvas->DrawItem(Item);
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

	// Hide world HUD chrome while dialogue / result overlays are up.
	if (PC && PC->HUDWidget && (PC->HUDWidget->IsDialogueActive() || PC->HUDWidget->IsResultScreenVisible()))
	{
		return;
	}

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

			if (Building->bIsCoreBuilding)
			{
				DrawProjectedLabel(PC, WorldLoc + FVector(0.f, 0.f, 40.f), TEXT("Chicken Coop"),
					FLinearColor(1.f, 0.9f, 0.35f));
			}
		}

		TArray<AActor*> Resources;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARTSResourceNode::StaticClass(), Resources);
		for (AActor* Actor : Resources)
		{
			if (!Actor)
			{
				continue;
			}
			const FVector WorldLoc = Actor->GetActorLocation() + FVector(0.f, 0.f, 120.f);
			DrawProjectedLabel(PC, WorldLoc, TEXT("Fodder Point"), FLinearColor(0.4f, 1.f, 0.45f));
		}
	}

	float Y = 40.f;
	const float X = 40.f;
	const float Line = 26.f;

	auto DrawLine = [&](const FString& Text, const FLinearColor& Color = FLinearColor::White)
	{
		FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), Font, Color);
		Item.EnableShadow(FLinearColor::Black);
		Item.Scale = FVector2D(1.1f, 1.1f);
		Canvas->DrawItem(Item);
		Y += Line;
	};

	if (!GM)
	{
		DrawLine(TEXT("ERROR: GameMode is NOT RTSGameMode"), FLinearColor::Red);
		DrawLine(TEXT("World Settings -> GameMode Override -> RTSGameMode / BP_RTSGameMode"), FLinearColor::Red);
		return;
	}

	const FText Status = GM->GetStatusText();
	if (!Status.IsEmpty())
	{
		DrawLine(Status.ToString(), GM->bVictory ? FLinearColor::Green : FLinearColor::Red);
	}

	if (PC && PC->bPlacementPending)
	{
		DrawLine(TEXT("Placement: click a lane on the map"), FLinearColor::Yellow);
	}

	Y = Canvas->SizeY - 40.f;
	DrawLine(TEXT("A/D Camera | Wheel Zoom | Esc/RMB cancel"), FLinearColor(0.65f, 0.65f, 0.65f));
}
