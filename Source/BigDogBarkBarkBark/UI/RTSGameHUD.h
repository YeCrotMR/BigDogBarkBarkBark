// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RTSUnitTypes.h"
#include "RTSGameHUD.generated.h"

class UTextBlock;
class UButton;
class UImage;
class UVerticalBox;
class UHorizontalBox;
class UCanvasPanel;
class UCanvasPanelSlot;
class UBorder;
class USizeBox;
class UFont;
class UTexture2D;
class ARTSGameMode;

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
	void ApplyDialogueDesignerLayout();
	void EnsureResultOverlay();
	void EnsureUpgradeToast();
	void RefreshUpgradeToastRestY();
	void TickUpgradeToast(float DeltaTime);
	static FString UnitTypeDisplayName(ERTSUnitType Type);
	static FString UpgradeBonusText(int32 NewLevel);
	void ApplyDialogueLine();
	void FinishDialogue();
	void EnterUIOnlyMode();
	void EnterGameAndUIMode();
	void LoadDialogueLines(ERTSDialogueKind Kind);
	void ApplyUnitIcons();
	void ApplyResourceIcons();
	void ApplyDesignerLayout();
	void ApplyDesignerChrome();
	void ResolveDesignerBindings();
	static void SetImageFromPath(UImage* Image, const TCHAR* TexturePath, float BrushSize = 48.f);
	static UTexture2D* LoadTextureFromPath(const TCHAR* TexturePath);
	static void CollectTextBlocks(UWidget* Root, TArray<UTextBlock*>& OutTexts);
	void EnsureHudStyleAssets();
	void ApplyHudFont(UTextBlock* Text, int32 Size) const;
	UTexture2D* CreateRoundPanelTexture();
	void ApplySoftPanelBrush(UBorder* Border, const FLinearColor& Tint);
	void ApplySoftButtonStyle(UButton* Btn, const FLinearColor& Normal, const FLinearColor& Hovered, const FLinearColor& Pressed);
	void EnsureWorldLabelLayer();
	void UpdateWorldLabels();
	UTextBlock* AcquireWorldLabel(int32 Index, const FString& Text, const FLinearColor& Color);
	UImage* EnsureIconAfterWidget(UWidget* BeforeWidget, UImage*& IconSlot, FName IconName, const TCHAR* TexturePath, float BrushSize);
	void RefreshRecruitSlot(UButton* Button, UTextBlock* CostText, int32 Cost, int32 AvailableFodder);
	void RefreshUpgradeSlot(UButton* Button, UTextBlock*& LevelText, UTextBlock*& CostText, UImage* CostIcon, ERTSUnitType Type, ARTSGameMode* GM);

	UBorder* WrapInColoredBorder(UWidget* Child, const FLinearColor& Color, const FMargin& InPadding, FName Name);

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
	UTextBlock* ObjectiveText = nullptr;

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

	/** Optional unit icons — name these exactly in WBP (or place Image inside each Btn*). */
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* IconRabbit = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* IconChicken = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* IconSheep = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* IconPig = nullptr;

	UPROPERTY()
	UImage* IconSoul = nullptr;

	UPROPERTY()
	UImage* IconFodderHeader = nullptr;

	UPROPERTY()
	UImage* IconFodderCostRabbit = nullptr;

	UPROPERTY()
	UImage* IconFodderCostChicken = nullptr;

	UPROPERTY()
	UImage* IconFodderCostSheep = nullptr;

	UPROPERTY()
	UImage* IconFodderCostPig = nullptr;

	UPROPERTY()
	UImage* IconSoulCostRabbit = nullptr;

	UPROPERTY()
	UImage* IconSoulCostChicken = nullptr;

	UPROPERTY()
	UImage* IconSoulCostSheep = nullptr;

	UPROPERTY()
	UImage* IconSoulCostPig = nullptr;

	UPROPERTY()
	UTextBlock* UpgradeCostRabbitText = nullptr;

	UPROPERTY()
	UTextBlock* UpgradeCostChickenText = nullptr;

	UPROPERTY()
	UTextBlock* UpgradeCostSheepText = nullptr;

	UPROPERTY()
	UTextBlock* UpgradeCostPigText = nullptr;

	// Dialogue overlay — prefer WBP BindWidget names; C++ EnsureDialogueOverlay is fallback.
	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* DialoguePanel = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* PortraitBlock = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* NamePlate = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* DialogueNameText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* DialogueBodyText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BtnDialogueAdvance = nullptr;

	/** Optional portrait image inside / instead of PortraitBlock (name: DialoguePortrait). */
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* DialoguePortrait = nullptr;

	// Result overlay (victory / defeat)
	UPROPERTY()
	UBorder* ResultPanel = nullptr;

	UPROPERTY()
	UBorder* ResultCard = nullptr;

	UPROPERTY()
	UBorder* ResultTitlePlate = nullptr;

	UPROPERTY()
	UTextBlock* ResultTitleText = nullptr;

	UPROPERTY()
	UTextBlock* ResultBodyText = nullptr;

	UPROPERTY()
	UButton* BtnPrimaryResult = nullptr;

	UPROPERTY()
	UTextBlock* PrimaryResultLabel = nullptr;

	UPROPERTY()
	UButton* BtnQuit = nullptr;

	UPROPERTY()
	UTextBlock* QuitResultLabel = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* WavePanel = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* ObjectivePanel = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	USizeBox* SideBarsBox = nullptr;

	/** Merged fodder+soul bar under Objective (built in ApplyDesignerChrome). */
	UPROPERTY()
	UBorder* ResourceBarPanel = nullptr;

	UPROPERTY()
	UBorder* UnitCardRabbit = nullptr;
	UPROPERTY()
	UBorder* UnitCardChicken = nullptr;
	UPROPERTY()
	UBorder* UnitCardSheep = nullptr;
	UPROPERTY()
	UBorder* UnitCardPig = nullptr;

	UPROPERTY()
	UFont* HudFont = nullptr;

	UPROPERTY()
	UTexture2D* RoundPanelTexture = nullptr;

	/** Screen-space world labels (Chicken Coop / Fodder Point) — Slate fonts stay sharp. */
	UPROPERTY()
	TArray<UTextBlock*> WorldLabelPool;

	UPROPERTY(meta = (BindWidgetOptional))
	UCanvasPanel* DialogueBottomChrome = nullptr;

	// Upgrade toast (drops from top center)
	UPROPERTY()
	UBorder* UpgradeToastPanel = nullptr;

	UPROPERTY()
	UBorder* UpgradeToastTitlePlate = nullptr;

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
	/** Rest Y below the wave bar (updated in RefreshUpgradeToastRestY). */
	float UpgradeToastRestY = 78.f;

	TArray<FString> DialogueSpeakers;
	TArray<FString> DialogueBodies;
};
