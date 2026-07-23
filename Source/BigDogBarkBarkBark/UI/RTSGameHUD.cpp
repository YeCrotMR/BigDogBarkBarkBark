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
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
	FLinearColor CardNormalBg(0.08f, 0.1f, 0.14f, 0.92f);
	FLinearColor CardSelectedBg(0.15f, 0.35f, 0.7f, 0.95f);
}

void URTSGameHUD::NativeConstruct()
{
	Super::NativeConstruct();
	BindFallbackButtonHandlers();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

TSharedRef<SWidget> URTSGameHUD::RebuildWidget()
{
	RebuildLayoutIfNeeded();
	EnsureResultOverlay();
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

	UHorizontalBox* TopBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TopBar"));
	UCanvasPanelSlot* TopSlot = Canvas->AddChildToCanvas(TopBar);
	TopSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
	TopSlot->SetOffsets(FMargin(10.f, 6.f, 10.f, 0.f));
	TopSlot->SetAutoSize(true);

	// Left: resources
	UBorder* LeftBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LeftBorder"));
	LeftBorder->SetBrushColor(FLinearColor(0.05f, 0.08f, 0.05f, 0.85f));
	LeftBorder->SetPadding(FMargin(8.f, 4.f));
	UHorizontalBoxSlot* LeftSlot = TopBar->AddChildToHorizontalBox(LeftBorder);
	LeftSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	LeftSlot->SetHorizontalAlignment(HAlign_Left);

	UVerticalBox* LeftCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftCol"));
	LeftBorder->SetContent(LeftCol);

	auto AddLeftLine = [&](const TCHAR* Name, UTextBlock*& OutText, const FString& Initial)
	{
		OutText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		OutText->SetText(FText::FromString(Initial));
		OutText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		LeftCol->AddChildToVerticalBox(OutText);
	};
	AddLeftLine(TEXT("FodderText"), FodderText, TEXT("Fodder: 0"));
	AddLeftLine(TEXT("SoulText"), SoulText, TEXT("Soul: 0"));
	AddLeftLine(TEXT("WaveText"), WaveText, TEXT("Wave: -"));
	AddLeftLine(TEXT("StatusText"), StatusText, TEXT(""));

	// Center: unit cards only
	UBorder* MidBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MidBorder"));
	MidBorder->SetBrushColor(FLinearColor(0.06f, 0.06f, 0.1f, 0.85f));
	MidBorder->SetPadding(FMargin(6.f, 4.f));
	UHorizontalBoxSlot* MidSlot = TopBar->AddChildToHorizontalBox(MidBorder);
	MidSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	MidSlot->SetPadding(FMargin(8.f, 0.f));

	UHorizontalBox* CardRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CardRow"));
	MidBorder->SetContent(CardRow);
	MakeUnitSlot(CardRow, TEXT("Rabbit"), TEXT("BtnRabbit"), BtnRabbit, CostRabbitText);
	MakeUnitSlot(CardRow, TEXT("Chicken"), TEXT("BtnChicken"), BtnChicken, CostChickenText);
	MakeUnitSlot(CardRow, TEXT("Sheep"), TEXT("BtnSheep"), BtnSheep, CostSheepText);
	MakeUnitSlot(CardRow, TEXT("Pig"), TEXT("BtnPig"), BtnPig, CostPigText);

	// Right: upgrades
	UBorder* RightBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RightBorder"));
	RightBorder->SetBrushColor(FLinearColor(0.1f, 0.06f, 0.14f, 0.88f));
	RightBorder->SetPadding(FMargin(8.f, 4.f));
	UHorizontalBoxSlot* RightSlot = TopBar->AddChildToHorizontalBox(RightBorder);
	RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	UVerticalBox* RightCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RightCol"));
	RightBorder->SetContent(RightCol);
	MakeUpgradeRow(RightCol, TEXT("Rabbit"), TEXT("BtnUpgradeRabbit"), BtnUpgradeRabbit, UpgradeRabbitText);
	MakeUpgradeRow(RightCol, TEXT("Chicken"), TEXT("BtnUpgradeChicken"), BtnUpgradeChicken, UpgradeChickenText);
	MakeUpgradeRow(RightCol, TEXT("Sheep"), TEXT("BtnUpgradeSheep"), BtnUpgradeSheep, UpgradeSheepText);
	MakeUpgradeRow(RightCol, TEXT("Pig"), TEXT("BtnUpgradePig"), BtnUpgradePig, UpgradePigText);
}

void URTSGameHUD::EnsureResultOverlay()
{
	if (DefeatPanel || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Canvas)
	{
		return;
	}

	DefeatPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DefeatPanel"));
	DefeatPanel->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.82f));
	DefeatPanel->SetPadding(FMargin(24.f));
	DefeatPanel->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(DefeatPanel);
	PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	PanelSlot->SetOffsets(FMargin(0.f));

	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DefeatCard"));
	Card->SetBrushColor(FLinearColor(0.18f, 0.05f, 0.05f, 0.96f));
	Card->SetPadding(FMargin(36.f, 28.f));
	DefeatPanel->SetContent(Card);

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DefeatCol"));
	Card->SetContent(Col);

	DefeatTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DefeatTitleText"));
	DefeatTitleText->SetText(FText::FromString(TEXT("失败\n鸡圈被摧毁了")));
	DefeatTitleText->SetJustification(ETextJustify::Center);
	DefeatTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.35f, 0.3f)));
	{
		UVerticalBoxSlot* DefeatTitleSlot = Col->AddChildToVerticalBox(DefeatTitleText);
		DefeatTitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));
		DefeatTitleSlot->SetHorizontalAlignment(HAlign_Center);
	}

	UHorizontalBox* BtnRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DefeatBtnRow"));
	{
		UVerticalBoxSlot* DefeatRowSlot = Col->AddChildToVerticalBox(BtnRow);
		DefeatRowSlot->SetHorizontalAlignment(HAlign_Center);
	}

	BtnReplay = MakeLabeledButton(BtnRow, TEXT("重玩"), TEXT("BtnReplay"));
	BtnQuit = MakeLabeledButton(BtnRow, TEXT("退出"), TEXT("BtnQuit"));
}

UVerticalBox* URTSGameHUD::MakeUnitSlot(UPanelWidget* Parent, const FString& Title, FName ButtonName, UButton*& OutButton, UTextBlock*& OutCostText)
{
	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Frame->SetBrushColor(CardNormalBg);
	Frame->SetPadding(FMargin(4.f));
	if (UHorizontalBox* HParent = Cast<UHorizontalBox>(Parent))
	{
		UHorizontalBoxSlot* BoxSlot = HParent->AddChildToHorizontalBox(Frame);
		BoxSlot->SetPadding(FMargin(3.f, 0.f));
		BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	else
	{
		Parent->AddChild(Frame);
	}

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Frame->SetContent(Col);

	UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NameText->SetText(FText::FromString(Title));
	NameText->SetJustification(ETextJustify::Center);
	Col->AddChildToVerticalBox(NameText);

	OutCostText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	OutCostText->SetText(FText::FromString(TEXT("Cost: ?")));
	OutCostText->SetJustification(ETextJustify::Center);
	OutCostText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 1.f, 0.6f)));
	Col->AddChildToVerticalBox(OutCostText);

	OutButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
	UTextBlock* PickLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	PickLabel->SetText(FText::FromString(TEXT("Select")));
	OutButton->AddChild(PickLabel);
	Col->AddChildToVerticalBox(OutButton);
	return Col;
}

UHorizontalBox* URTSGameHUD::MakeUpgradeRow(UPanelWidget* Parent, const FString& Title, FName ButtonName, UButton*& OutButton, UTextBlock*& OutLevelText)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBox* VParent = Cast<UVerticalBox>(Parent))
	{
		UVerticalBoxSlot* BoxSlot = VParent->AddChildToVerticalBox(Row);
		BoxSlot->SetPadding(FMargin(0.f, 1.f));
	}
	else
	{
		Parent->AddChild(Row);
	}

	OutLevelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	OutLevelText->SetText(FText::FromString(FString::Printf(TEXT("%s Lv0"), *Title)));
	UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(OutLevelText);
	TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

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
	if (BtnReplay) { BtnReplay->OnClicked.AddDynamic(this, &URTSGameHUD::OnReplay); }
	if (BtnQuit) { BtnQuit->OnClicked.AddDynamic(this, &URTSGameHUD::OnQuit); }
}

void URTSGameHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshTexts();
	RefreshCardHighlights();
	RefreshResultOverlay();
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
		FodderText->SetText(FText::FromString(FString::Printf(TEXT("Fodder: %d"), GM->Fodder)));
	}
	if (SoulText)
	{
		SoulText->SetText(FText::FromString(FString::Printf(TEXT("Soul: %d"), GM->Soul)));
	}
	if (WaveText)
	{
		if (ARTSWaveManager* WM = GM->GetWaveManager())
		{
			WaveText->SetText(FText::FromString(FString::Printf(
				TEXT("Wave: %d  |  Enemies: %d"), WM->CurrentWaveIndex + 1, WM->AliveEnemies)));
		}
		else
		{
			WaveText->SetText(FText::FromString(TEXT("Wave: -")));
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
		Style.Hovered.TintColor = FSlateColor(bSel ? CardSelectedBg : FLinearColor(0.2f, 0.25f, 0.35f, 0.95f));
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

void URTSGameHUD::ShowDefeatScreen()
{
	EnsureResultOverlay();
	bDefeatShown = false;
	RefreshResultOverlay();
}

void URTSGameHUD::RefreshResultOverlay()
{
	EnsureResultOverlay();
	ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));
	if (!DefeatPanel || !GM)
	{
		return;
	}

	const bool bShowDefeat = GM->bGameOver && !GM->bVictory;
	if (bShowDefeat)
	{
		if (!bDefeatShown)
		{
			bDefeatShown = true;
			DefeatPanel->SetVisibility(ESlateVisibility::Visible);
			if (DefeatTitleText)
			{
				DefeatTitleText->SetText(FText::FromString(TEXT("失败\n鸡圈被摧毁了")));
			}

			if (APlayerController* PC = GetOwningPlayer())
			{
				FInputModeUIOnly InputMode;
				InputMode.SetWidgetToFocus(TakeWidget());
				InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
				PC->SetInputMode(InputMode);
				PC->bShowMouseCursor = true;
			}
		}
	}
	else if (DefeatPanel->GetVisibility() != ESlateVisibility::Collapsed)
	{
		DefeatPanel->SetVisibility(ESlateVisibility::Collapsed);
		bDefeatShown = false;
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

void URTSGameHUD::OnQuit()
{
	APlayerController* PC = GetOwningPlayer();
	UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
}
