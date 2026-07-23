// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSGameHUD.h"
#include "RTSGameMode.h"
#include "RTSPlayerController.h"
#include "RTSWaveManager.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
	FLinearColor HeaderBarBg(0.55f, 0.82f, 0.95f, 0.95f);
	FLinearColor PanelBodyBg(0.95f, 0.78f, 0.55f, 0.92f);
	FLinearColor WaveBarBg(0.95f, 0.95f, 0.95f, 0.95f);
	FLinearColor CardNormalBg(0.92f, 0.72f, 0.48f, 0.95f);
	FLinearColor CardSelectedBg(0.35f, 0.65f, 1.f, 0.95f);
	FLinearColor DarkText(0.08f, 0.08f, 0.1f, 1.f);
}

void URTSGameHUD::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureDialogueOverlay();
	EnsureResultOverlay();
	EnsureUpgradeToast();
	BindFallbackButtonHandlers();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

TSharedRef<SWidget> URTSGameHUD::RebuildWidget()
{
	RebuildLayoutIfNeeded();
	EnsureDialogueOverlay();
	EnsureResultOverlay();
	EnsureUpgradeToast();
	return Super::RebuildWidget();
}

void URTSGameHUD::RebuildLayoutIfNeeded()
{
	if (FodderText && BtnRabbit && BtnUpgradeRabbit)
	{
		return;
	}
	if (!WidgetTree || bBuiltFallbackLayout)
	{
		return;
	}
	bBuiltFallbackLayout = true;

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Canvas;

	UBorder* WaveBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WaveBorder"));
	WavePanel = WaveBorder;
	WaveBorder->SetBrushColor(WaveBarBg);
	WaveBorder->SetPadding(FMargin(18.f, 6.f));
	{
		UCanvasPanelSlot* WaveSlot = Canvas->AddChildToCanvas(WaveBorder);
		WaveSlot->SetAnchors(FAnchors(0.5f, 0.f, 0.5f, 0.f));
		WaveSlot->SetAlignment(FVector2D(0.5f, 0.f));
		WaveSlot->SetAutoSize(true);
		WaveSlot->SetOffsets(FMargin(0.f, 10.f, 0.f, 0.f));
	}

	UVerticalBox* WaveCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("WaveCol"));
	WaveBorder->SetContent(WaveCol);

	WaveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WaveText"));
	WaveText->SetText(FText::FromString(TEXT("Wave -  |  Enemies 0")));
	WaveText->SetColorAndOpacity(FSlateColor(DarkText));
	WaveText->SetJustification(ETextJustify::Center);
	WaveCol->AddChildToVerticalBox(WaveText);

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetText(FText::GetEmpty());
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.25f, 0.25f, 0.28f)));
	StatusText->SetJustification(ETextJustify::Center);
	WaveCol->AddChildToVerticalBox(StatusText);

	// Objective — own canvas slot so width is independent of soul/fodder.
	ObjectivePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ObjectivePanel"));
	ObjectivePanel->SetBrushColor(FLinearColor(1.f, 1.f, 1.f, 0.95f));
	ObjectivePanel->SetPadding(FMargin(10.f, 8.f));
	{
		UCanvasPanelSlot* ObjSlot = Canvas->AddChildToCanvas(ObjectivePanel);
		ObjSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
		ObjSlot->SetAlignment(FVector2D(0.f, 0.f));
		ObjSlot->SetAutoSize(true);
		ObjSlot->SetOffsets(FMargin(12.f, 12.f, 0.f, 0.f));
	}

	UTextBlock* ObjectiveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ObjectiveText"));
	ObjectiveText->SetText(FText::FromString(TEXT("Objective: Protect the Chicken Coop")));
	ObjectiveText->SetColorAndOpacity(FSlateColor(DarkText));
	ObjectiveText->SetJustification(ETextJustify::Left);
	ObjectiveText->SetAutoWrapText(true);
	ObjectivePanel->SetContent(ObjectiveText);

	// Narrow soul + fodder column.
	SideBarsBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SideBarsBox"));
	SideBarsBox->SetWidthOverride(350.f);
	{
		UCanvasPanelSlot* SideSlot = Canvas->AddChildToCanvas(SideBarsBox);
		SideSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
		SideSlot->SetAlignment(FVector2D(0.f, 0.f));
		SideSlot->SetAutoSize(true);
		SideSlot->SetOffsets(FMargin(12.f, 72.f, 0.f, 0.f));
	}

	UVerticalBox* LeftSidebar = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftSidebar"));
	SideBarsBox->AddChild(LeftSidebar);

	UBorder* SoulPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SoulPanel"));
	SoulPanel->SetBrushColor(PanelBodyBg);
	SoulPanel->SetPadding(FMargin(0.f));
	{
		UVerticalBoxSlot* SoulPanelSlot = LeftSidebar->AddChildToVerticalBox(SoulPanel);
		SoulPanelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	}

	UVerticalBox* SoulCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SoulCol"));
	SoulPanel->SetContent(SoulCol);

	MakeHeaderBar(SoulCol, TEXT("Souls 0"), SoulText, TEXT("SoulText"));

	UBorder* UpgradeBody = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("UpgradeBody"));
	UpgradeBody->SetBrushColor(PanelBodyBg);
	UpgradeBody->SetPadding(FMargin(8.f, 6.f));
	SoulCol->AddChildToVerticalBox(UpgradeBody);

	UVerticalBox* UpgradeCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("UpgradeCol"));
	UpgradeBody->SetContent(UpgradeCol);

	UTextBlock* UpgradeTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UpgradeTitle"));
	UpgradeTitle->SetText(FText::FromString(TEXT("Soul Upgrades")));
	UpgradeTitle->SetColorAndOpacity(FSlateColor(DarkText));
	UpgradeTitle->SetJustification(ETextJustify::Center);
	{
		UVerticalBoxSlot* TitleSlot = UpgradeCol->AddChildToVerticalBox(UpgradeTitle);
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	}

	MakeUpgradeRow(UpgradeCol, TEXT("Rabbit"), TEXT("BtnUpgradeRabbit"), BtnUpgradeRabbit, UpgradeRabbitText);
	MakeUpgradeRow(UpgradeCol, TEXT("Chicken"), TEXT("BtnUpgradeChicken"), BtnUpgradeChicken, UpgradeChickenText);
	MakeUpgradeRow(UpgradeCol, TEXT("Sheep"), TEXT("BtnUpgradeSheep"), BtnUpgradeSheep, UpgradeSheepText);
	MakeUpgradeRow(UpgradeCol, TEXT("Pig"), TEXT("BtnUpgradePig"), BtnUpgradePig, UpgradePigText);

	UBorder* FodderPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FodderPanel"));
	FodderPanel->SetBrushColor(PanelBodyBg);
	FodderPanel->SetPadding(FMargin(0.f));
	LeftSidebar->AddChildToVerticalBox(FodderPanel);

	UVerticalBox* FodderCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FodderCol"));
	FodderPanel->SetContent(FodderCol);

	MakeHeaderBar(FodderCol, TEXT("Fodder 0"), FodderText, TEXT("FodderText"));

	UBorder* SlotBody = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SlotBody"));
	SlotBody->SetBrushColor(PanelBodyBg);
	SlotBody->SetPadding(FMargin(6.f));
	FodderCol->AddChildToVerticalBox(SlotBody);

	UVerticalBox* SlotCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SlotCol"));
	SlotBody->SetContent(SlotCol);

	MakeUnitSlot(SlotCol, TEXT("Rabbit"), TEXT("BtnRabbit"), BtnRabbit, CostRabbitText);
	MakeUnitSlot(SlotCol, TEXT("Chicken"), TEXT("BtnChicken"), BtnChicken, CostChickenText);
	MakeUnitSlot(SlotCol, TEXT("Sheep"), TEXT("BtnSheep"), BtnSheep, CostSheepText);
	MakeUnitSlot(SlotCol, TEXT("Pig"), TEXT("BtnPig"), BtnPig, CostPigText);
}

void URTSGameHUD::EnsureDialogueOverlay()
{
	if (DialoguePanel || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Canvas)
	{
		return;
	}

	DialoguePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DialoguePanel"));
	// Light dim so the gameplay camera remains the dialogue backdrop.
	DialoguePanel->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.2f));
	DialoguePanel->SetPadding(FMargin(0.f));
	DialoguePanel->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(DialoguePanel);
	PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	PanelSlot->SetOffsets(FMargin(0.f));

	UCanvasPanel* Inner = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DialogueInner"));
	DialoguePanel->SetContent(Inner);

	// Full-screen click catcher
	BtnDialogueAdvance = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BtnDialogueAdvance"));
	{
		FButtonStyle Style = BtnDialogueAdvance->WidgetStyle;
		Style.Normal.TintColor = FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.01f));
		Style.Hovered.TintColor = FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.01f));
		Style.Pressed.TintColor = FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.01f));
		BtnDialogueAdvance->SetStyle(Style);
	}
	{
		UCanvasPanelSlot* AdvSlot = Inner->AddChildToCanvas(BtnDialogueAdvance);
		AdvSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		AdvSlot->SetOffsets(FMargin(0.f));
	}

	// Portrait placeholder (立绘) — gameplay camera remains the backdrop.
	PortraitBlock = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PortraitBlock"));
	// Warm color so it stays visible against green grass.
	PortraitBlock->SetBrushColor(FLinearColor(0.95f, 0.75f, 0.35f, 0.98f));
	PortraitBlock->SetPadding(FMargin(12.f));
	{
		UCanvasPanelSlot* PortSlot = Inner->AddChildToCanvas(PortraitBlock);
		PortSlot->SetAnchors(FAnchors(0.5f, 1.f, 0.5f, 1.f));
		PortSlot->SetAlignment(FVector2D(0.5f, 1.f));
		PortSlot->SetSize(FVector2D(240.f, 340.f));
		PortSlot->SetPosition(FVector2D(0.f, -150.f));
		PortSlot->SetZOrder(5);
	}
	UTextBlock* PortraitLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PortraitLabel"));
	PortraitLabel->SetText(FText::FromString(TEXT("Portrait")));
	PortraitLabel->SetJustification(ETextJustify::Center);
	PortraitLabel->SetColorAndOpacity(FSlateColor(DarkText));
	PortraitBlock->SetContent(PortraitLabel);

	// Bottom chrome: name plate sits on the top edge of the white dialogue box.
	DialogueBottomChrome = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DialogueBottomChrome"));
	{
		UCanvasPanelSlot* ChromeSlot = Inner->AddChildToCanvas(DialogueBottomChrome);
		ChromeSlot->SetAnchors(FAnchors(0.06f, 1.f, 0.94f, 1.f));
		ChromeSlot->SetAlignment(FVector2D(0.f, 1.f));
		ChromeSlot->SetOffsets(FMargin(0.f, -24.f, 0.f, 24.f));
		ChromeSlot->SetSize(FVector2D(0.f, 140.f));
		ChromeSlot->SetZOrder(10);
	}

	UBorder* DialogueBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DialogueBox"));
	DialogueBox->SetBrushColor(FLinearColor(1.f, 1.f, 1.f, 0.97f));
	DialogueBox->SetPadding(FMargin(20.f, 22.f, 20.f, 14.f));
	{
		UCanvasPanelSlot* BoxSlot = DialogueBottomChrome->AddChildToCanvas(DialogueBox);
		BoxSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BoxSlot->SetOffsets(FMargin(0.f, 18.f, 0.f, 0.f));
	}

	UVerticalBox* BoxCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DialogueBoxCol"));
	DialogueBox->SetContent(BoxCol);

	DialogueBodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DialogueBodyText"));
	DialogueBodyText->SetText(FText::FromString(TEXT("...")));
	DialogueBodyText->SetColorAndOpacity(FSlateColor(DarkText));
	DialogueBodyText->SetAutoWrapText(true);
	{
		UVerticalBoxSlot* BodySlot = BoxCol->AddChildToVerticalBox(DialogueBodyText);
		BodySlot->SetPadding(FMargin(0.f, 4.f, 0.f, 8.f));
		BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UTextBlock* ContinueHint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ContinueHint"));
	ContinueHint->SetText(FText::FromString(TEXT("Click / Space to continue")));
	ContinueHint->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.4f, 0.45f)));
	ContinueHint->SetJustification(ETextJustify::Right);
	BoxCol->AddChildToVerticalBox(ContinueHint);

	NamePlate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NamePlate"));
	NamePlate->SetBrushColor(FLinearColor(1.f, 0.92f, 0.55f, 0.98f));
	NamePlate->SetPadding(FMargin(16.f, 6.f));
	{
		UCanvasPanelSlot* NameSlot = DialogueBottomChrome->AddChildToCanvas(NamePlate);
		NameSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
		NameSlot->SetAlignment(FVector2D(0.f, 0.5f));
		NameSlot->SetAutoSize(true);
		NameSlot->SetPosition(FVector2D(16.f, 18.f));
	}
	DialogueNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DialogueNameText"));
	DialogueNameText->SetText(FText::FromString(TEXT("Hen")));
	DialogueNameText->SetColorAndOpacity(FSlateColor(DarkText));
	NamePlate->SetContent(DialogueNameText);
}

void URTSGameHUD::EnsureResultOverlay()
{
	if (ResultPanel || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Canvas)
	{
		return;
	}

	ResultPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResultPanel"));
	ResultPanel->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.82f));
	ResultPanel->SetPadding(FMargin(24.f));
	ResultPanel->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(ResultPanel);
	PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	PanelSlot->SetOffsets(FMargin(0.f));

	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResultCard"));
	Card->SetBrushColor(FLinearColor(0.12f, 0.1f, 0.1f, 0.96f));
	Card->SetPadding(FMargin(36.f, 28.f));
	ResultPanel->SetContent(Card);

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ResultCol"));
	Card->SetContent(Col);

	ResultTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResultTitleText"));
	ResultTitleText->SetText(FText::FromString(TEXT("Defeat")));
	ResultTitleText->SetJustification(ETextJustify::Center);
	ResultTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.35f, 0.3f)));
	{
		UVerticalBoxSlot* TitleSlot = Col->AddChildToVerticalBox(ResultTitleText);
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
	}

	UHorizontalBox* BtnRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ResultBtnRow"));
	{
		UVerticalBoxSlot* RowSlot = Col->AddChildToVerticalBox(BtnRow);
		RowSlot->SetHorizontalAlignment(HAlign_Center);
	}

	BtnPrimaryResult = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BtnPrimaryResult"));
	PrimaryResultLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PrimaryResultLabel"));
	PrimaryResultLabel->SetText(FText::FromString(TEXT("Replay")));
	BtnPrimaryResult->AddChild(PrimaryResultLabel);
	{
		UHorizontalBoxSlot* Added = BtnRow->AddChildToHorizontalBox(BtnPrimaryResult);
		Added->SetPadding(FMargin(3.f, 0.f));
	}

	BtnQuit = MakeLabeledButton(BtnRow, TEXT("Quit"), TEXT("BtnQuit"));
}

void URTSGameHUD::LoadDialogueLines(ERTSDialogueKind Kind)
{
	DialogueSpeakers.Reset();
	DialogueBodies.Reset();

	auto Add = [&](const TCHAR* Speaker, const TCHAR* Body)
	{
		DialogueSpeakers.Add(Speaker);
		DialogueBodies.Add(Body);
	};

	switch (Kind)
	{
	case ERTSDialogueKind::Intro:
		Add(TEXT("Hen"), TEXT("Protect the chicken coop!"));
		Add(TEXT("Hen"), TEXT("Recruit animals and hold the lanes."));
		break;
	case ERTSDialogueKind::Victory:
		Add(TEXT("Hen"), TEXT("We did it! The fox-wolf pack is gone."));
		break;
	case ERTSDialogueKind::Defeat:
		Add(TEXT("Hen"), TEXT("Oh no... the coop was destroyed."));
		break;
	}
}

void URTSGameHUD::SetGameplayHudVisible(bool bVisible)
{
	const ESlateVisibility Vis = bVisible
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Collapsed;

	if (WavePanel)
	{
		WavePanel->SetVisibility(Vis);
	}
	if (ObjectivePanel)
	{
		ObjectivePanel->SetVisibility(Vis);
	}
	if (SideBarsBox)
	{
		SideBarsBox->SetVisibility(Vis);
	}
}

FString URTSGameHUD::UnitTypeDisplayName(ERTSUnitType Type)
{
	switch (Type)
	{
	case ERTSUnitType::Rabbit: return TEXT("Rabbit");
	case ERTSUnitType::Chicken: return TEXT("Chicken");
	case ERTSUnitType::Sheep: return TEXT("Sheep");
	case ERTSUnitType::Pig: return TEXT("Pig");
	default: return TEXT("Unit");
	}
}

FString URTSGameHUD::UpgradeBonusText(int32 NewLevel)
{
	switch (NewLevel)
	{
	case 1: return TEXT("HP +50%");
	case 2: return TEXT("ATK +50%");
	case 3: return TEXT("Fodder Cost -20%");
	default: return TEXT("");
	}
}

void URTSGameHUD::EnsureUpgradeToast()
{
	if (UpgradeToastPanel || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Canvas)
	{
		return;
	}

	UpgradeToastPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("UpgradeToastPanel"));
	UpgradeToastPanel->SetBrushColor(FLinearColor(0.12f, 0.18f, 0.28f, 0.94f));
	UpgradeToastPanel->SetPadding(FMargin(22.f, 14.f));
	UpgradeToastPanel->SetVisibility(ESlateVisibility::Collapsed);
	UpgradeToastPanel->SetRenderOpacity(1.f);

	UpgradeToastSlot = Canvas->AddChildToCanvas(UpgradeToastPanel);
	UpgradeToastSlot->SetAnchors(FAnchors(0.5f, 0.f));
	UpgradeToastSlot->SetAlignment(FVector2D(0.5f, 0.f));
	UpgradeToastSlot->SetAutoSize(true);
	UpgradeToastSlot->SetPosition(FVector2D(0.f, -140.f));
	UpgradeToastSlot->SetZOrder(50);

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("UpgradeToastCol"));
	UpgradeToastPanel->SetContent(Col);

	UpgradeToastTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UpgradeToastTitle"));
	UpgradeToastTitleText->SetText(FText::FromString(TEXT("Upgraded")));
	UpgradeToastTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.92f, 0.45f, 1.f)));
	UpgradeToastTitleText->SetJustification(ETextJustify::Center);
	{
		UVerticalBoxSlot* TitleSlot = Col->AddChildToVerticalBox(UpgradeToastTitleText);
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
	}

	UpgradeToastBodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UpgradeToastBody"));
	UpgradeToastBodyText->SetText(FText::FromString(TEXT("")));
	UpgradeToastBodyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.97f, 1.f, 1.f)));
	UpgradeToastBodyText->SetJustification(ETextJustify::Center);
	{
		UVerticalBoxSlot* BodySlot = Col->AddChildToVerticalBox(UpgradeToastBodyText);
		BodySlot->SetHorizontalAlignment(HAlign_Center);
	}
}

void URTSGameHUD::ShowUpgradeToast(ERTSUnitType Type, int32 NewLevel)
{
	EnsureUpgradeToast();
	if (!UpgradeToastPanel || !UpgradeToastTitleText || !UpgradeToastBodyText)
	{
		return;
	}

	const FString Name = UnitTypeDisplayName(Type);
	const FString Bonus = UpgradeBonusText(NewLevel);
	UpgradeToastTitleText->SetText(FText::FromString(
		FString::Printf(TEXT("%s  →  Lv%d"), *Name, NewLevel)));
	UpgradeToastBodyText->SetText(FText::FromString(
		FString::Printf(TEXT("Bonus: %s"), *Bonus)));

	UpgradeToastAge = 0.f;
	bUpgradeToastActive = true;
	UpgradeToastPanel->SetRenderOpacity(1.f);
	UpgradeToastPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UpgradeToastSlot)
	{
		UpgradeToastSlot->SetPosition(FVector2D(0.f, -140.f));
	}
}

void URTSGameHUD::TickUpgradeToast(float DeltaTime)
{
	if (!bUpgradeToastActive || !UpgradeToastPanel || !UpgradeToastSlot)
	{
		return;
	}

	UpgradeToastAge += DeltaTime;

	constexpr float DropDuration = 0.35f;
	constexpr float HoldDuration = 2.2f;
	constexpr float FadeDuration = 0.45f;
	const float Total = DropDuration + HoldDuration + FadeDuration;

	if (UpgradeToastAge >= Total)
	{
		bUpgradeToastActive = false;
		UpgradeToastPanel->SetVisibility(ESlateVisibility::Collapsed);
		UpgradeToastPanel->SetRenderOpacity(1.f);
		return;
	}

	float Y = UpgradeToastRestY;
	float Opacity = 1.f;

	if (UpgradeToastAge < DropDuration)
	{
		const float T = UpgradeToastAge / DropDuration;
		const float Ease = 1.f - FMath::Square(1.f - T); // ease-out
		Y = FMath::Lerp(-140.f, UpgradeToastRestY, Ease);
	}
	else if (UpgradeToastAge > DropDuration + HoldDuration)
	{
		const float FadeT = (UpgradeToastAge - DropDuration - HoldDuration) / FadeDuration;
		Opacity = 1.f - FMath::Clamp(FadeT, 0.f, 1.f);
		Y = UpgradeToastRestY - FadeT * 24.f;
	}

	UpgradeToastSlot->SetPosition(FVector2D(0.f, Y));
	UpgradeToastPanel->SetRenderOpacity(Opacity);
}

void URTSGameHUD::PlayDialogueSequence(ERTSDialogueKind Kind)
{
	EnsureDialogueOverlay();
	EnsureResultOverlay();

	if (ResultPanel)
	{
		ResultPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	bResultShown = false;

	ActiveDialogueKind = Kind;
	LoadDialogueLines(Kind);
	DialogueLineIndex = 0;
	bDialogueActive = true;

	SetGameplayHudVisible(false);

	if (DialoguePanel)
	{
		DialoguePanel->SetVisibility(ESlateVisibility::Visible);
	}

	UGameplayStatics::SetGamePaused(this, true);
	EnterUIOnlyMode();
	ApplyDialogueLine();
}

void URTSGameHUD::ApplyDialogueLine()
{
	if (!DialogueBodies.IsValidIndex(DialogueLineIndex))
	{
		FinishDialogue();
		return;
	}

	if (DialogueNameText)
	{
		DialogueNameText->SetText(FText::FromString(
			DialogueSpeakers.IsValidIndex(DialogueLineIndex) ? DialogueSpeakers[DialogueLineIndex] : TEXT("?")));
	}
	if (DialogueBodyText)
	{
		DialogueBodyText->SetText(FText::FromString(DialogueBodies[DialogueLineIndex]));
	}
}

void URTSGameHUD::AdvanceDialogue()
{
	if (!bDialogueActive)
	{
		return;
	}

	++DialogueLineIndex;
	if (DialogueLineIndex >= DialogueBodies.Num())
	{
		FinishDialogue();
	}
	else
	{
		ApplyDialogueLine();
	}
}

void URTSGameHUD::FinishDialogue()
{
	bDialogueActive = false;
	if (DialoguePanel)
	{
		DialoguePanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));

	switch (ActiveDialogueKind)
	{
	case ERTSDialogueKind::Intro:
		UGameplayStatics::SetGamePaused(this, false);
		EnterGameAndUIMode();
		SetGameplayHudVisible(true);
		if (GM)
		{
			GM->StartWavesIfNeeded();
		}
		break;
	case ERTSDialogueKind::Victory:
		ShowResultScreen(true);
		break;
	case ERTSDialogueKind::Defeat:
		ShowResultScreen(false);
		break;
	}
}

void URTSGameHUD::ShowResultScreen(bool bVictory)
{
	EnsureResultOverlay();
	bResultIsVictory = bVictory;
	bResultShown = true;

	if (DialoguePanel)
	{
		DialoguePanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (ResultPanel)
	{
		ResultPanel->SetVisibility(ESlateVisibility::Visible);
	}

	if (ResultTitleText)
	{
		if (bVictory)
		{
			ResultTitleText->SetText(FText::FromString(TEXT("Victory\nFox-Wolf Coalition Repelled!")));
			ResultTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.95f, 0.4f)));
		}
		else
		{
			ResultTitleText->SetText(FText::FromString(TEXT("Defeat\nThe chicken coop was destroyed")));
			ResultTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.35f, 0.3f)));
		}
	}

	if (PrimaryResultLabel)
	{
		PrimaryResultLabel->SetText(FText::FromString(bVictory ? TEXT("Next Level") : TEXT("Replay")));
	}

	UGameplayStatics::SetGamePaused(this, true);
	EnterUIOnlyMode();
}

void URTSGameHUD::ShowDefeatScreen()
{
	PlayDialogueSequence(ERTSDialogueKind::Defeat);
}

void URTSGameHUD::EnterUIOnlyMode()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
}

void URTSGameHUD::EnterGameAndUIMode()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
}

UBorder* URTSGameHUD::MakeHeaderBar(UPanelWidget* Parent, const FString& Title, UTextBlock*& OutText, FName TextName)
{
	UBorder* Header = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Header->SetBrushColor(HeaderBarBg);
	Header->SetPadding(FMargin(10.f, 6.f));
	if (UVerticalBox* VParent = Cast<UVerticalBox>(Parent))
	{
		VParent->AddChildToVerticalBox(Header);
	}
	else
	{
		Parent->AddChild(Header);
	}

	OutText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TextName);
	OutText->SetText(FText::FromString(Title));
	OutText->SetColorAndOpacity(FSlateColor(DarkText));
	OutText->SetJustification(ETextJustify::Center);
	Header->SetContent(OutText);
	return Header;
}

void URTSGameHUD::MakeUnitSlot(UPanelWidget* Parent, const FString& Title, FName ButtonName, UButton*& OutButton, UTextBlock*& OutCostText)
{
	OutButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
	{
		FButtonStyle Style = OutButton->WidgetStyle;
		Style.Normal.TintColor = FSlateColor(CardNormalBg);
		Style.Hovered.TintColor = FSlateColor(FLinearColor(1.f, 0.85f, 0.6f, 0.95f));
		Style.Pressed.TintColor = FSlateColor(CardSelectedBg);
		OutButton->SetStyle(Style);
	}

	if (UVerticalBox* VParent = Cast<UVerticalBox>(Parent))
	{
		UVerticalBoxSlot* BoxSlot = VParent->AddChildToVerticalBox(OutButton);
		BoxSlot->SetPadding(FMargin(0.f, 3.f));
	}
	else if (UHorizontalBox* HParent = Cast<UHorizontalBox>(Parent))
	{
		UHorizontalBoxSlot* BoxSlot = HParent->AddChildToHorizontalBox(OutButton);
		BoxSlot->SetPadding(FMargin(3.f, 0.f));
		BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	else
	{
		Parent->AddChild(OutButton);
	}

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	OutButton->AddChild(Row);

	UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NameText->SetText(FText::FromString(Title));
	NameText->SetColorAndOpacity(FSlateColor(DarkText));
	{
		UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(NameText);
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetPadding(FMargin(8.f, 6.f));
		NameSlot->SetVerticalAlignment(VAlign_Center);
	}

	OutCostText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	OutCostText->SetText(FText::FromString(TEXT("?")));
	OutCostText->SetColorAndOpacity(FSlateColor(FLinearColor(0.15f, 0.45f, 0.2f)));
	{
		UHorizontalBoxSlot* CostSlot = Row->AddChildToHorizontalBox(OutCostText);
		CostSlot->SetPadding(FMargin(8.f, 6.f));
		CostSlot->SetVerticalAlignment(VAlign_Center);
	}
}

UHorizontalBox* URTSGameHUD::MakeUpgradeRow(UPanelWidget* Parent, const FString& Title, FName ButtonName, UButton*& OutButton, UTextBlock*& OutLevelText)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBox* VParent = Cast<UVerticalBox>(Parent))
	{
		UVerticalBoxSlot* BoxSlot = VParent->AddChildToVerticalBox(Row);
		BoxSlot->SetPadding(FMargin(0.f, 2.f));
	}
	else
	{
		Parent->AddChild(Row);
	}

	OutLevelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	OutLevelText->SetText(FText::FromString(FString::Printf(TEXT("%s Lv0"), *Title)));
	OutLevelText->SetColorAndOpacity(FSlateColor(DarkText));
	UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(OutLevelText);
	TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TextSlot->SetVerticalAlignment(VAlign_Center);

	OutButton = MakeLabeledButton(Row, TEXT("Up"), ButtonName);
	return Row;
}

UButton* URTSGameHUD::MakeLabeledButton(UPanelWidget* Parent, const FString& Label, FName Name)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(FText::FromString(Label));
	Button->AddChild(LabelText);
	if (UHorizontalBox* HParent = Cast<UHorizontalBox>(Parent))
	{
		if (UHorizontalBoxSlot* Added = HParent->AddChildToHorizontalBox(Button))
		{
			Added->SetPadding(FMargin(3.f, 0.f));
			Added->SetVerticalAlignment(VAlign_Center);
		}
	}
	else
	{
		Parent->AddChild(Button);
	}
	return Button;
}

void URTSGameHUD::BindFallbackButtonHandlers()
{
	if (BtnRabbit) { BtnRabbit->OnClicked.AddDynamic(this, &URTSGameHUD::OnRecruitRabbit); }
	if (BtnChicken) { BtnChicken->OnClicked.AddDynamic(this, &URTSGameHUD::OnRecruitChicken); }
	if (BtnSheep) { BtnSheep->OnClicked.AddDynamic(this, &URTSGameHUD::OnRecruitSheep); }
	if (BtnPig) { BtnPig->OnClicked.AddDynamic(this, &URTSGameHUD::OnRecruitPig); }
	if (BtnUpgradeRabbit) { BtnUpgradeRabbit->OnClicked.AddDynamic(this, &URTSGameHUD::OnUpgradeRabbit); }
	if (BtnUpgradeChicken) { BtnUpgradeChicken->OnClicked.AddDynamic(this, &URTSGameHUD::OnUpgradeChicken); }
	if (BtnUpgradeSheep) { BtnUpgradeSheep->OnClicked.AddDynamic(this, &URTSGameHUD::OnUpgradeSheep); }
	if (BtnUpgradePig) { BtnUpgradePig->OnClicked.AddDynamic(this, &URTSGameHUD::OnUpgradePig); }
	if (BtnDialogueAdvance) { BtnDialogueAdvance->OnClicked.AddDynamic(this, &URTSGameHUD::OnDialogueClicked); }
	if (BtnPrimaryResult) { BtnPrimaryResult->OnClicked.AddDynamic(this, &URTSGameHUD::OnPrimaryResult); }
	if (BtnQuit) { BtnQuit->OnClicked.AddDynamic(this, &URTSGameHUD::OnQuit); }
}

void URTSGameHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshTexts();
	RefreshCardHighlights();
	TickUpgradeToast(InDeltaTime);
}

void URTSGameHUD::RefreshTexts()
{
	ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM)
	{
		return;
	}

	if (FodderText)
	{
		FodderText->SetText(FText::FromString(FString::Printf(TEXT("Fodder  %d"), GM->Fodder)));
	}
	if (SoulText)
	{
		SoulText->SetText(FText::FromString(FString::Printf(TEXT("Souls  %d"), GM->Soul)));
	}
	if (WaveText)
	{
		if (ARTSWaveManager* WM = GM->GetWaveManager())
		{
			WaveText->SetText(FText::FromString(FString::Printf(
				TEXT("Wave %d  |  Enemies %d"), WM->CurrentWaveIndex + 1, WM->AliveEnemies)));
		}
		else
		{
			WaveText->SetText(FText::FromString(TEXT("Wave -  |  Enemies 0")));
		}
	}
	if (StatusText)
	{
		StatusText->SetText(GM->GetStatusText());
	}

	auto SetCost = [&](UTextBlock* Text, ERTSUnitType Type)
	{
		if (Text)
		{
			Text->SetText(FText::FromString(FString::Printf(TEXT("%d"), GM->GetEffectiveFodderCost(Type))));
		}
	};
	SetCost(CostRabbitText, ERTSUnitType::Rabbit);
	SetCost(CostChickenText, ERTSUnitType::Chicken);
	SetCost(CostSheepText, ERTSUnitType::Sheep);
	SetCost(CostPigText, ERTSUnitType::Pig);

	auto SetUpgrade = [&](UTextBlock* Text, UButton* Btn, ERTSUnitType Type, const TCHAR* Name)
	{
		const int32 Lv = GM->GetUnitUpgradeLevel(Type);
		if (Text)
		{
			if (Lv >= 3)
			{
				Text->SetText(FText::FromString(FString::Printf(TEXT("%s Lv3"), Name)));
			}
			else
			{
				Text->SetText(FText::FromString(FString::Printf(
					TEXT("%s Lv%d (%d)"), Name, Lv, GM->GetUpgradeCost(Lv + 1))));
			}
		}
		if (Btn)
		{
			Btn->SetIsEnabled(Lv < 3 && GM->Soul >= GM->GetUpgradeCost(Lv + 1));
		}
	};
	SetUpgrade(UpgradeRabbitText, BtnUpgradeRabbit, ERTSUnitType::Rabbit, TEXT("Rabbit"));
	SetUpgrade(UpgradeChickenText, BtnUpgradeChicken, ERTSUnitType::Chicken, TEXT("Chicken"));
	SetUpgrade(UpgradeSheepText, BtnUpgradeSheep, ERTSUnitType::Sheep, TEXT("Sheep"));
	SetUpgrade(UpgradePigText, BtnUpgradePig, ERTSUnitType::Pig, TEXT("Pig"));
}

void URTSGameHUD::RefreshCardHighlights()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	auto Tint = [&](UButton* Btn, ERTSUnitType Type)
	{
		if (!Btn)
		{
			return;
		}
		const bool bSel = PC && PC->bPlacementPending && PC->PendingRecruitType == Type;
		FButtonStyle Style = Btn->WidgetStyle;
		Style.Normal.TintColor = FSlateColor(bSel ? CardSelectedBg : CardNormalBg);
		Style.Hovered.TintColor = FSlateColor(bSel ? CardSelectedBg : FLinearColor(1.f, 0.85f, 0.6f, 0.95f));
		Btn->SetStyle(Style);
	};
	Tint(BtnRabbit, ERTSUnitType::Rabbit);
	Tint(BtnChicken, ERTSUnitType::Chicken);
	Tint(BtnSheep, ERTSUnitType::Sheep);
	Tint(BtnPig, ERTSUnitType::Pig);
}

void URTSGameHUD::OnRecruitRabbit()
{
	if (ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer()))
	{
		PC->SelectRecruitCard(ERTSUnitType::Rabbit);
	}
}
void URTSGameHUD::OnRecruitChicken()
{
	if (ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer()))
	{
		PC->SelectRecruitCard(ERTSUnitType::Chicken);
	}
}
void URTSGameHUD::OnRecruitSheep()
{
	if (ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer()))
	{
		PC->SelectRecruitCard(ERTSUnitType::Sheep);
	}
}
void URTSGameHUD::OnRecruitPig()
{
	if (ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer()))
	{
		PC->SelectRecruitCard(ERTSUnitType::Pig);
	}
}
void URTSGameHUD::OnUpgradeRabbit()
{
	if (ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer()))
	{
		PC->UpgradeUnitType(ERTSUnitType::Rabbit);
	}
}
void URTSGameHUD::OnUpgradeChicken()
{
	if (ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer()))
	{
		PC->UpgradeUnitType(ERTSUnitType::Chicken);
	}
}
void URTSGameHUD::OnUpgradeSheep()
{
	if (ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer()))
	{
		PC->UpgradeUnitType(ERTSUnitType::Sheep);
	}
}
void URTSGameHUD::OnUpgradePig()
{
	if (ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer()))
	{
		PC->UpgradeUnitType(ERTSUnitType::Pig);
	}
}

void URTSGameHUD::OnDialogueClicked()
{
	AdvanceDialogue();
}

void URTSGameHUD::OnPrimaryResult()
{
	if (bResultIsVictory)
	{
		OnNextLevel();
	}
	else
	{
		OnReplay();
	}
}

void URTSGameHUD::OnReplay()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UGameplayStatics::SetGamePaused(World, false);
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(World, true);
	UGameplayStatics::OpenLevel(World, FName(*LevelName));
}

void URTSGameHUD::OnNextLevel()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UGameplayStatics::SetGamePaused(World, false);
	ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));
	FName LevelToOpen = NAME_None;
	if (GM && !GM->NextLevelName.IsNone())
	{
		LevelToOpen = GM->NextLevelName;
	}
	else
	{
		LevelToOpen = FName(*UGameplayStatics::GetCurrentLevelName(World, true));
	}
	UGameplayStatics::OpenLevel(World, LevelToOpen);
}

void URTSGameHUD::OnQuit()
{
	APlayerController* PC = GetOwningPlayer();
	UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
}
