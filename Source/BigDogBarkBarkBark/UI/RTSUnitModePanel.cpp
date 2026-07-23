// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSUnitModePanel.h"
#include "RTSUnitModeRing.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
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
	RefreshModeHighlight(EUnitWorkMode::Combat);
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

	USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSize"));
	RootSize->SetWidthOverride(250.f);
	RootSize->SetHeightOverride(52.f);
	WidgetTree->RootWidget = RootSize;

	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Frame"));
	Frame->SetBrushColor(FLinearColor(0.05f, 0.05f, 0.08f, 0.85f));
	Frame->SetPadding(FMargin(6.f, 4.f));
	RootSize->AddChild(Frame);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ModeRow"));
	Frame->SetContent(Row);

	auto MakeBtn = [&](const FString& Label, FName Name) -> UButton*
	{
		UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(FText::FromString(Label));
		Text->SetJustification(ETextJustify::Center);
		Btn->AddChild(Text);
		if (UHorizontalBoxSlot* Added = Row->AddChildToHorizontalBox(Btn))
		{
			Added->SetPadding(FMargin(4.f, 0.f));
			Added->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Added->SetHorizontalAlignment(HAlign_Fill);
			Added->SetVerticalAlignment(VAlign_Fill);
		}
		return Btn;
	};

	BtnCombat = MakeBtn(TEXT("Combat"), TEXT("BtnCombat"));
	BtnCollect = MakeBtn(TEXT("Collect"), TEXT("BtnCollect"));
}

void URTSUnitModePanel::RefreshModeHighlight(EUnitWorkMode Mode)
{
	const FLinearColor Active(0.15f, 0.55f, 1.f, 1.f);
	const FLinearColor Idle(0.2f, 0.2f, 0.24f, 0.95f);
	auto Tint = [&](UButton* Btn, bool bOn)
	{
		if (!Btn)
		{
			return;
		}
		FButtonStyle Style = Btn->WidgetStyle;
		Style.Normal.TintColor = FSlateColor(bOn ? Active : Idle);
		Style.Hovered.TintColor = FSlateColor(bOn ? Active : FLinearColor(0.35f, 0.35f, 0.4f, 1.f));
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
