// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RTSUnitTypes.h"
#include "RTSGameHUD.generated.h"

class UTextBlock;
class UButton;
class UVerticalBox;
class UHorizontalBox;
class UCanvasPanel;
class UBorder;

UCLASS()
class BIGDOGBARKBARKBARK_API URTSGameHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "RTS|UI")
	void ShowDefeatScreen();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	void RebuildLayoutIfNeeded();
	void BindFallbackButtonHandlers();
	void RefreshTexts();
	void RefreshCardHighlights();
	void EnsureResultOverlay();
	void RefreshResultOverlay();

	UButton* MakeLabeledButton(UPanelWidget* Parent, const FString& Label, FName Name);
	UVerticalBox* MakeUnitSlot(UPanelWidget* Parent, const FString& Title, FName ButtonName, UButton*& OutButton, UTextBlock*& OutCostText);
	UHorizontalBox* MakeUpgradeRow(UPanelWidget* Parent, const FString& Title, FName ButtonName, UButton*& OutButton, UTextBlock*& OutLevelText);

	UFUNCTION() void OnRecruitRabbit();
	UFUNCTION() void OnRecruitChicken();
	UFUNCTION() void OnRecruitSheep();
	UFUNCTION() void OnRecruitPig();
	UFUNCTION() void OnUpgradeRabbit();
	UFUNCTION() void OnUpgradeChicken();
	UFUNCTION() void OnUpgradeSheep();
	UFUNCTION() void OnUpgradePig();
	UFUNCTION() void OnReplay();
	UFUNCTION() void OnQuit();

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* FodderText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* SoulText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* WaveText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* StatusText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BtnRabbit = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BtnChicken = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BtnSheep = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BtnPig = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* CostRabbitText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* CostChickenText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* CostSheepText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* CostPigText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BtnUpgradeRabbit = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BtnUpgradeChicken = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BtnUpgradeSheep = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BtnUpgradePig = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* UpgradeRabbitText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* UpgradeChickenText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* UpgradeSheepText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* UpgradePigText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* DefeatPanel = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* DefeatTitleText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BtnReplay = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BtnQuit = nullptr;

	bool bBuiltFallbackLayout = false;
	bool bDefeatShown = false;
};
