// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSUnitModePanel.h"
#include "RTSUnitModeRing.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetTree.h"

void URTSUnitModePanel::SetOwnerRing(ARTSUnitModeRing* InRing)
{
	OwnerRing = InRing;
}

void URTSUnitModePanel::NativeConstruct()
{
	Super::NativeConstruct();
	if (BtnCombat)
	{
		BtnCombat->OnClicked.AddDynamic(this, &URTSUnitModePanel::OnCombatClicked);
	}
	if (BtnCollect)
	{
		BtnCollect->OnClicked.AddDynamic(this, &URTSUnitModePanel::OnCollectClicked);
	}
}

TSharedRef<SWidget> URTSUnitModePanel::RebuildWidget()
{
	BuildLayout();
	return Super::RebuildWidget();
}

void URTSUnitModePanel::BuildLayout()
{
	if (bBuilt || !WidgetTree)
	{
		return;
	}
	bBuilt = true;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ModeRow"));
	WidgetTree->RootWidget = Row;

	auto MakeBtn = [&](const FString& Label, FName Name) -> UButton*
	{
		UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(FText::FromString(Label));
		Btn->AddChild(Text);
		if (UHorizontalBoxSlot* Added = Row->AddChildToHorizontalBox(Btn))
		{
			Added->SetPadding(FMargin(6.f, 0.f));
		}
		return Btn;
	};

	BtnCombat = MakeBtn(TEXT("战斗"), TEXT("BtnCombat"));
	BtnCollect = MakeBtn(TEXT("采集"), TEXT("BtnCollect"));
}

void URTSUnitModePanel::RefreshModeHighlight(EUnitWorkMode Mode)
{
	const FLinearColor Active(0.15f, 0.55f, 1.f, 1.f);
	const FLinearColor Idle(0.15f, 0.15f, 0.18f, 0.95f);
	auto Tint = [&](UButton* Btn, bool bOn)
	{
		if (!Btn)
		{
			return;
		}
		FButtonStyle Style = Btn->WidgetStyle;
		Style.Normal.TintColor = FSlateColor(bOn ? Active : Idle);
		Style.Hovered.TintColor = FSlateColor(bOn ? Active : FLinearColor(0.25f, 0.25f, 0.3f, 1.f));
		Btn->SetStyle(Style);
	};
	Tint(BtnCombat, Mode == EUnitWorkMode::Combat);
	Tint(BtnCollect, Mode == EUnitWorkMode::Collect);
}

void URTSUnitModePanel::OnCombatClicked()
{
	if (ARTSUnitModeRing* Ring = OwnerRing.Get())
	{
		Ring->ApplyWorkMode(EUnitWorkMode::Combat);
	}
}

void URTSUnitModePanel::OnCollectClicked()
{
	if (ARTSUnitModeRing* Ring = OwnerRing.Get())
	{
		Ring->ApplyWorkMode(EUnitWorkMode::Collect);
	}
}
