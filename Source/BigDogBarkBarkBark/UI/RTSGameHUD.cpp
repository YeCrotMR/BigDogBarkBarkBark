// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSGameHUD.h"
#include "RTSGameMode.h"
#include "RTSPlayerController.h"
#include "RTSUnitBase.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"

void URTSGameHUD::NativeConstruct()
{
	Super::NativeConstruct();
	RebuildLayout();
}

void URTSGameHUD::RebuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = Canvas;

	RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
	UCanvasPanelSlot* RootSlot = Canvas->AddChildToCanvas(RootBox);
	RootSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
	RootSlot->SetPosition(FVector2D(20.f, 20.f));
	RootSlot->SetAutoSize(true);

	FodderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FodderText"));
	FodderText->SetText(FText::FromString(TEXT("Fodder: 0")));
	RootBox->AddChildToVerticalBox(FodderText);

	SelectionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectionText"));
	SelectionText->SetText(FText::FromString(TEXT("Lane: 0 | Select a farm unit to switch mode")));
	RootBox->AddChildToVerticalBox(SelectionText);

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	RootBox->AddChildToVerticalBox(StatusText);

	UHorizontalBox* RecruitRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RecruitRow"));
	RootBox->AddChildToVerticalBox(RecruitRow);
	MakeButton(RecruitRow, TEXT("Rabbit(12)"), TEXT("BtnRabbit"))->OnClicked.AddDynamic(this, &URTSGameHUD::OnRecruitRabbit);
	MakeButton(RecruitRow, TEXT("Chicken(12)"), TEXT("BtnChicken"))->OnClicked.AddDynamic(this, &URTSGameHUD::OnRecruitChicken);
	MakeButton(RecruitRow, TEXT("Sheep(20)"), TEXT("BtnSheep"))->OnClicked.AddDynamic(this, &URTSGameHUD::OnRecruitSheep);
	MakeButton(RecruitRow, TEXT("Pig(30)"), TEXT("BtnPig"))->OnClicked.AddDynamic(this, &URTSGameHUD::OnRecruitPig);

	UHorizontalBox* LaneRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("LaneRow"));
	RootBox->AddChildToVerticalBox(LaneRow);
	MakeButton(LaneRow, TEXT("Lane 0"), TEXT("BtnLane0"))->OnClicked.AddDynamic(this, &URTSGameHUD::OnLane0);
	MakeButton(LaneRow, TEXT("Lane 1"), TEXT("BtnLane1"))->OnClicked.AddDynamic(this, &URTSGameHUD::OnLane1);

	UHorizontalBox* ModeRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ModeRow"));
	RootBox->AddChildToVerticalBox(ModeRow);
	MakeButton(ModeRow, TEXT("Combat Mode"), TEXT("BtnCombat"))->OnClicked.AddDynamic(this, &URTSGameHUD::OnModeCombat);
	MakeButton(ModeRow, TEXT("Collect Mode"), TEXT("BtnCollect"))->OnClicked.AddDynamic(this, &URTSGameHUD::OnModeCollect);
}

UButton* URTSGameHUD::MakeButton(UPanelWidget* Parent, const FString& Label, FName Name)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(FText::FromString(Label));
	Button->AddChild(LabelText);
	Parent->AddChild(Button);
	return Button;
}

void URTSGameHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshTexts();
}

void URTSGameHUD::RefreshTexts()
{
	ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (FodderText && GM)
	{
		FodderText->SetText(FText::FromString(FString::Printf(TEXT("Fodder: %d"), GM->Fodder)));
	}
	if (StatusText && GM)
	{
		StatusText->SetText(GM->GetStatusText());
	}
	if (SelectionText && PC)
	{
		FString ModeName = TEXT("None");
		if (PC->SelectedUnit && PC->SelectedUnit->IsAlive())
		{
			ModeName = PC->SelectedUnit->WorkMode == EUnitWorkMode::Collect ? TEXT("Collect") : TEXT("Combat");
		}
		SelectionText->SetText(FText::FromString(FString::Printf(
			TEXT("Lane: %d | Selected mode: %s | Keys: 1-4 recruit, Q/E lane, T toggle mode"),
			PC->SelectedLaneIndex,
			*ModeName)));
	}
}

void URTSGameHUD::OnRecruitRabbit()
{
	if (ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer()))
	{
		PC->RecruitType(ERTSUnitType::Rabbit);
	}
}

void URTSGameHUD::OnRecruitChicken()
{
	if (ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer()))
	{
		PC->RecruitType(ERTSUnitType::Chicken);
	}
}

void URTSGameHUD::OnRecruitSheep()
{
	if (ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer()))
	{
		PC->RecruitType(ERTSUnitType::Sheep);
	}
}

void URTSGameHUD::OnRecruitPig()
{
	if (ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer()))
	{
		PC->RecruitType(ERTSUnitType::Pig);
	}
}

void URTSGameHUD::OnLane0()
{
	if (ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer()))
	{
		PC->SetSelectedLane(0);
	}
}

void URTSGameHUD::OnLane1()
{
	if (ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer()))
	{
		PC->SetSelectedLane(1);
	}
}

void URTSGameHUD::OnModeCombat()
{
	if (ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer()))
	{
		PC->SetSelectedUnitWorkMode(EUnitWorkMode::Combat);
	}
}

void URTSGameHUD::OnModeCollect()
{
	if (ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer()))
	{
		PC->SetSelectedUnitWorkMode(EUnitWorkMode::Collect);
	}
}
