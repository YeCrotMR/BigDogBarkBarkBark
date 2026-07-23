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
class UCanvasPanelSlot;
class UBorder;
class USizeBox;

UENUM(BlueprintType)
enum class ERTSDialogueKind : uint8
{
	Intro,
	Victory,
	Defeat
};

UCLASS()
class BIGDOGBARKBARKBARK_API URTSGameHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "RTS|UI")
	void PlayDialogueSequence(ERTSDialogueKind Kind);

	UFUNCTION(BlueprintCallable, Category = "RTS|UI")
	void AdvanceDialogue();

	UFUNCTION(BlueprintCallable, Category = "RTS|UI")
	bool IsDialogueActive() const { return bDialogueActive; }

	UFUNCTION(BlueprintCallable, Category = "RTS|UI")
	bool IsResultScreenVisible() const { return bResultShown; }

	UFUNCTION(BlueprintCallable, Category = "RTS|UI")
	void ShowResultScreen(bool bVictory);

	/** Legacy entry — routes to defeat dialogue if needed. Prefer PlayDialogueSequence. */
	UFUNCTION(BlueprintCallable, Category = "RTS|UI")
	void ShowDefeatScreen();

	void SetGameplayHudVisible(bool bVisible);

	/** Top-center drop toast after a successful soul upgrade. */
	UFUNCTION(BlueprintCallable, Category = "RTS|UI")
	void ShowUpgradeToast(ERTSUnitType Type, int32 NewLevel);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	void RebuildLayoutIfNeeded();
	void BindFallbackButtonHandlers();
	void RefreshTexts();
	void RefreshCardHighlights();
	void EnsureDialogueOverlay();
	void EnsureResultOverlay();
	void EnsureUpgradeToast();
	void TickUpgradeToast(float DeltaTime);
	static FString UnitTypeDisplayName(ERTSUnitType Type);
	static FString UpgradeBonusText(int32 NewLevel);
	void ApplyDialogueLine();
	void FinishDialogue();
	void EnterUIOnlyMode();
	void EnterGameAndUIMode();
	void LoadDialogueLines(ERTSDialogueKind Kind);

	UButton* MakeLabeledButton(UPanelWidget* Parent, const FString& Label, FName Name);
	UBorder* MakeHeaderBar(UPanelWidget* Parent, const FString& Title, UTextBlock*& OutText, FName TextName);
	void MakeUnitSlot(UPanelWidget* Parent, const FString& Title, FName ButtonName, UButton*& OutButton, UTextBlock*& OutCostText);
	UHorizontalBox* MakeUpgradeRow(UPanelWidget* Parent, const FString& Title, FName ButtonName, UButton*& OutButton, UTextBlock*& OutLevelText);

	UFUNCTION() void OnRecruitRabbit();
	UFUNCTION() void OnRecruitChicken();
	UFUNCTION() void OnRecruitSheep();
	UFUNCTION() void OnRecruitPig();
	UFUNCTION() void OnUpgradeRabbit();
	UFUNCTION() void OnUpgradeChicken();
	UFUNCTION() void OnUpgradeSheep();
	UFUNCTION() void OnUpgradePig();
	UFUNCTION() void OnDialogueClicked();
	UFUNCTION() void OnPrimaryResult();
	UFUNCTION() void OnReplay();
	UFUNCTION() void OnNextLevel();
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

	// Dialogue overlay
	UPROPERTY()
	UBorder* DialoguePanel = nullptr;

	UPROPERTY()
	UBorder* PortraitBlock = nullptr;

	UPROPERTY()
	UBorder* NamePlate = nullptr;

	UPROPERTY()
	UTextBlock* DialogueNameText = nullptr;

	UPROPERTY()
	UTextBlock* DialogueBodyText = nullptr;

	UPROPERTY()
	UButton* BtnDialogueAdvance = nullptr;

	// Result overlay (victory / defeat)
	UPROPERTY()
	UBorder* ResultPanel = nullptr;

	UPROPERTY()
	UTextBlock* ResultTitleText = nullptr;

	UPROPERTY()
	UButton* BtnPrimaryResult = nullptr;

	UPROPERTY()
	UTextBlock* PrimaryResultLabel = nullptr;

	UPROPERTY()
	UButton* BtnQuit = nullptr;

	UPROPERTY()
	UBorder* WavePanel = nullptr;

	UPROPERTY()
	UBorder* ObjectivePanel = nullptr;

	UPROPERTY()
	USizeBox* SideBarsBox = nullptr;

	UPROPERTY()
	UCanvasPanel* DialogueBottomChrome = nullptr;

	// Upgrade toast (drops from top center)
	UPROPERTY()
	UBorder* UpgradeToastPanel = nullptr;

	UPROPERTY()
	UTextBlock* UpgradeToastTitleText = nullptr;

	UPROPERTY()
	UTextBlock* UpgradeToastBodyText = nullptr;

	UPROPERTY()
	UCanvasPanelSlot* UpgradeToastSlot = nullptr;

	bool bBuiltFallbackLayout = false;
	bool bDialogueActive = false;
	bool bResultShown = false;
	bool bResultIsVictory = false;
	bool bUpgradeToastActive = false;
	ERTSDialogueKind ActiveDialogueKind = ERTSDialogueKind::Intro;
	int32 DialogueLineIndex = 0;
	float UpgradeToastAge = 0.f;
	float UpgradeToastRestY = 28.f;

	TArray<FString> DialogueSpeakers;
	TArray<FString> DialogueBodies;
};
