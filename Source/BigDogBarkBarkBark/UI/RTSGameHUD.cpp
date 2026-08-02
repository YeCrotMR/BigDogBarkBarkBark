// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSGameHUD.h"
#include "RTSGameMode.h"
#include "RTSPlayerController.h"
#include "RTSWaveManager.h"
#include "RTSBaseBuilding.h"
#include "RTSResourceNode.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Spacer.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Engine/Font.h"
#include "TextureResource.h"

namespace
{
	// Farm storybook — info/cards stay readable (near-opaque); buttons may be slightly softer.
	FLinearColor InfoBarBg(0.55f, 0.82f, 0.95f, 0.96f);
	FLinearColor PanelBodyBg(0.98f, 0.94f, 0.86f, 0.96f);
	FLinearColor CardNormalBg(0.94f, 0.82f, 0.58f, 0.96f);
	FLinearColor CardSelectedBg(0.40f, 0.70f, 1.f, 0.98f);
	FLinearColor UpgradeBtnBg(0.28f, 0.42f, 0.72f, 0.92f);
	FLinearColor UpgradeBtnHover(0.36f, 0.52f, 0.85f, 0.95f);
	FLinearColor DispatchBtnBg(0.18f, 0.62f, 0.32f, 0.94f);
	FLinearColor DispatchBtnHover(0.24f, 0.74f, 0.40f, 0.96f);
	FLinearColor IconBadgeBg(0.98f, 0.95f, 0.88f, 0.98f);
	FLinearColor NamePlateBg(1.f, 0.92f, 0.55f, 0.98f);
	FLinearColor DialogBoxBg(1.f, 1.f, 1.f, 0.97f);
	FLinearColor ModalCardBg(0.98f, 0.95f, 0.88f, 0.98f);
	FLinearColor DarkText(0.08f, 0.08f, 0.1f, 1.f);
	FLinearColor CostOnDark(1.f, 0.96f, 0.88f, 1.f);
	FLinearColor QuitBtnBg(0.45f, 0.48f, 0.55f, 0.94f);
	FLinearColor QuitBtnHover(0.55f, 0.58f, 0.65f, 0.96f);
	constexpr float RecruitIconSize = 84.f;
	constexpr float SideBarWidth = 280.f;
	constexpr float ActionBtnMinHeight = 38.f;
	constexpr float DialoguePortraitSize = 400.f;
	constexpr float DialogueNameBand = 64.f;
	constexpr float DialogueSideMargin = 40.f;
	constexpr float DialogueBottomMargin = 24.f;
	constexpr float DialogueGap = 12.f;
	constexpr int32 FontTitle = 18;
	constexpr int32 FontBody = 16;
	constexpr int32 FontSmall = 14;
	constexpr int32 FontDialogueName = 28;
	constexpr int32 FontDialogueBody = 26;
	constexpr int32 FontWorldLabel = 24;
	constexpr int32 FontModalTitle = 22;
}

void URTSGameHUD::NativeConstruct()
{
	Super::NativeConstruct();
	// Resolve WBP bindings first so dialogue/result overlays skip C++ fallback when designed.
	ResolveDesignerBindings();
	EnsureHudStyleAssets();
	EnsureDialogueOverlay();
	EnsureResultOverlay();
	EnsureUpgradeToast();
	BindFallbackButtonHandlers();
	ApplyDesignerLayout();
	ApplyDesignerChrome();
	ApplyUnitIcons();
	ApplyResourceIcons();
	EnsureWorldLabelLayer();
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
	// Designer WBP provides BindWidget controls — skip C++ fallback layout.
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
	WaveBorder->SetBrushColor(InfoBarBg);
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

	UTextBlock* ObjectiveLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ObjectiveText"));
	ObjectiveLabel->SetText(FText::FromString(TEXT("Objective: Protect the Chicken Coop")));
	ObjectiveLabel->SetColorAndOpacity(FSlateColor(DarkText));
	ObjectiveLabel->SetJustification(ETextJustify::Left);
	ObjectiveLabel->SetAutoWrapText(true);
	ObjectivePanel->SetContent(ObjectiveLabel);
	ObjectiveText = ObjectiveLabel;

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
	if (!WidgetTree)
	{
		return;
	}

	// Designer WBP (WBP_RTSGameHUD): use authored widgets when present.
	if (!DialoguePanel)
	{
		DialoguePanel = Cast<UBorder>(WidgetTree->FindWidget(TEXT("DialoguePanel")));
	}
	if (!DialogueNameText)
	{
		DialogueNameText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("DialogueNameText")));
	}
	if (!DialogueBodyText)
	{
		DialogueBodyText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("DialogueBodyText")));
	}
	if (!BtnDialogueAdvance)
	{
		BtnDialogueAdvance = Cast<UButton>(WidgetTree->FindWidget(TEXT("BtnDialogueAdvance")));
	}
	if (!PortraitBlock)
	{
		PortraitBlock = Cast<UBorder>(WidgetTree->FindWidget(TEXT("PortraitBlock")));
	}
	if (!NamePlate)
	{
		NamePlate = Cast<UBorder>(WidgetTree->FindWidget(TEXT("NamePlate")));
	}
	if (!DialogueBottomChrome)
	{
		DialogueBottomChrome = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("DialogueBottomChrome")));
	}
	if (!DialoguePortrait)
	{
		DialoguePortrait = Cast<UImage>(WidgetTree->FindWidget(TEXT("DialoguePortrait")));
	}

	if (DialoguePanel)
	{
		DialoguePanel->SetVisibility(ESlateVisibility::Collapsed);
		ApplyDialogueDesignerLayout();
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

	// Left 500 portrait; name overlays portrait bottom; dialogue same height to the right.
	PortraitBlock = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PortraitBlock"));
	ApplySoftPanelBrush(PortraitBlock, IconBadgeBg);
	PortraitBlock->SetPadding(FMargin(8.f));
	{
		UCanvasPanelSlot* PortSlot = Inner->AddChildToCanvas(PortraitBlock);
		PortSlot->SetAnchors(FAnchors(0.f, 1.f));
		PortSlot->SetAlignment(FVector2D(0.f, 1.f));
		PortSlot->SetSize(FVector2D(DialoguePortraitSize, DialoguePortraitSize));
		PortSlot->SetPosition(FVector2D(DialogueSideMargin, -DialogueBottomMargin));
		PortSlot->SetZOrder(5);
	}
	DialoguePortrait = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DialoguePortrait"));
	SetImageFromPath(DialoguePortrait, TEXT("/Game/UI/Icons/T_Icon_Chicken.T_Icon_Chicken"), DialoguePortraitSize - 24.f);
	PortraitBlock->SetContent(DialoguePortrait);

	NamePlate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NamePlate"));
	ApplySoftPanelBrush(NamePlate, NamePlateBg);
	NamePlate->SetPadding(FMargin(12.f, 8.f));
	{
		UCanvasPanelSlot* NameSlot = Inner->AddChildToCanvas(NamePlate);
		NameSlot->SetAnchors(FAnchors(0.f, 1.f));
		NameSlot->SetAlignment(FVector2D(0.f, 1.f));
		NameSlot->SetAutoSize(false);
		NameSlot->SetSize(FVector2D(DialoguePortraitSize, DialogueNameBand));
		NameSlot->SetPosition(FVector2D(DialogueSideMargin, -DialogueBottomMargin));
		NameSlot->SetZOrder(6);
	}
	DialogueNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DialogueNameText"));
	DialogueNameText->SetText(FText::FromString(TEXT("Hen")));
	DialogueNameText->SetColorAndOpacity(FSlateColor(DarkText));
	DialogueNameText->SetJustification(ETextJustify::Center);
	NamePlate->SetContent(DialogueNameText);

	DialogueBottomChrome = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DialogueBottomChrome"));
	{
		// Bottom-anchored: Offsets.Bottom is HEIGHT (= portrait height).
		UCanvasPanelSlot* ChromeSlot = Inner->AddChildToCanvas(DialogueBottomChrome);
		ChromeSlot->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f));
		ChromeSlot->SetAlignment(FVector2D(0.f, 1.f));
		ChromeSlot->SetAutoSize(false);
		ChromeSlot->SetOffsets(FMargin(
			DialogueSideMargin + DialoguePortraitSize + DialogueGap,
			-DialogueBottomMargin,
			DialogueSideMargin,
			DialoguePortraitSize));
		ChromeSlot->SetZOrder(10);
	}

	UBorder* DialogueBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DialogueBox"));
	ApplySoftPanelBrush(DialogueBox, DialogBoxBg);
	DialogueBox->SetPadding(FMargin(28.f, 24.f, 28.f, 18.f));
	{
		UCanvasPanelSlot* BoxSlot = DialogueBottomChrome->AddChildToCanvas(DialogueBox);
		BoxSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BoxSlot->SetAutoSize(false);
		BoxSlot->SetOffsets(FMargin(0.f));
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

	// Hint pinned to dialogue chrome bottom-right (not stacked under body).
	UTextBlock* ContinueHint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ContinueHint"));
	ContinueHint->SetText(FText::FromString(TEXT("Click / Space to continue")));
	ContinueHint->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.4f, 0.45f)));
	ContinueHint->SetJustification(ETextJustify::Right);
	{
		UCanvasPanelSlot* HintSlot = DialogueBottomChrome->AddChildToCanvas(ContinueHint);
		HintSlot->SetAnchors(FAnchors(1.f, 1.f));
		HintSlot->SetAlignment(FVector2D(1.f, 1.f));
		HintSlot->SetAutoSize(true);
		HintSlot->SetPosition(FVector2D(-28.f, -18.f));
		HintSlot->SetZOrder(12);
	}

	ApplyDialogueDesignerLayout();
}

void URTSGameHUD::ApplyDialogueDesignerLayout()
{
	if (!DialoguePanel)
	{
		return;
	}

	EnsureHudStyleAssets();

	// Dim backdrop (designer Borders default to solid white).
	DialoguePanel->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.2f));
	DialoguePanel->SetPadding(FMargin(0.f));

	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(DialoguePanel->Slot))
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		PanelSlot->SetOffsets(FMargin(0.f));
		PanelSlot->SetAlignment(FVector2D(0.f, 0.f));
		PanelSlot->SetZOrder(50);
	}

	if (BtnDialogueAdvance)
	{
		FButtonStyle Style = BtnDialogueAdvance->WidgetStyle;
		const FSlateColor Invisible(FLinearColor(0.f, 0.f, 0.f, 0.f));
		Style.Normal.TintColor = Invisible;
		Style.Hovered.TintColor = Invisible;
		Style.Pressed.TintColor = Invisible;
		Style.Disabled.TintColor = Invisible;
		Style.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
		BtnDialogueAdvance->SetStyle(Style);
		BtnDialogueAdvance->SetBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.f));
		BtnDialogueAdvance->SetVisibility(ESlateVisibility::Visible);
		if (UCanvasPanelSlot* AdvSlot = Cast<UCanvasPanelSlot>(BtnDialogueAdvance->Slot))
		{
			AdvSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			AdvSlot->SetOffsets(FMargin(0.f));
			AdvSlot->SetZOrder(0);
		}
	}

	if (!DialoguePortrait && WidgetTree)
	{
		DialoguePortrait = Cast<UImage>(WidgetTree->FindWidget(TEXT("DialoguePortrait")));
	}
	if (!DialoguePortrait && PortraitBlock && WidgetTree)
	{
		DialoguePortrait = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DialoguePortrait"));
		PortraitBlock->SetContent(DialoguePortrait);
	}
	if (DialoguePortrait)
	{
		SetImageFromPath(DialoguePortrait, TEXT("/Game/UI/Icons/T_Icon_Chicken.T_Icon_Chicken"), DialoguePortraitSize - 24.f);
		DialoguePortrait->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (PortraitBlock)
	{
		ApplySoftPanelBrush(PortraitBlock, IconBadgeBg);
		PortraitBlock->SetPadding(FMargin(8.f));
		PortraitBlock->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (DialoguePortrait && PortraitBlock->GetContent() != DialoguePortrait)
		{
			PortraitBlock->SetContent(DialoguePortrait);
		}
	}

	UCanvasPanel* DialogueInner = Cast<UCanvasPanel>(DialoguePanel->GetContent());

	// Portrait bottom-left, 500×500.
	UWidget* PortraitWidget = PortraitBlock ? static_cast<UWidget*>(PortraitBlock) : static_cast<UWidget*>(DialoguePortrait);
	if (PortraitWidget)
	{
		if (UCanvasPanelSlot* PortSlot = Cast<UCanvasPanelSlot>(PortraitWidget->Slot))
		{
			PortSlot->SetAnchors(FAnchors(0.f, 1.f));
			PortSlot->SetAlignment(FVector2D(0.f, 1.f));
			PortSlot->SetAutoSize(false);
			PortSlot->SetSize(FVector2D(DialoguePortraitSize, DialoguePortraitSize));
			PortSlot->SetPosition(FVector2D(DialogueSideMargin, -DialogueBottomMargin));
			PortSlot->SetZOrder(5);
		}
	}

	// Name plate covers the bottom of the portrait.
	if (NamePlate && DialogueInner)
	{
		ApplySoftPanelBrush(NamePlate, NamePlateBg);
		NamePlate->SetPadding(FMargin(12.f, 8.f));
		NamePlate->SetVisibility(ESlateVisibility::HitTestInvisible);
		NamePlate->RemoveFromParent();
		UCanvasPanelSlot* NameSlot = DialogueInner->AddChildToCanvas(NamePlate);
		NameSlot->SetAnchors(FAnchors(0.f, 1.f));
		NameSlot->SetAlignment(FVector2D(0.f, 1.f));
		NameSlot->SetAutoSize(false);
		NameSlot->SetSize(FVector2D(DialoguePortraitSize, DialogueNameBand));
		NameSlot->SetPosition(FVector2D(DialogueSideMargin, -DialogueBottomMargin));
		NameSlot->SetZOrder(6);
	}

	// Dialogue column: same height as portrait, to the right.
	if (DialogueBottomChrome)
	{
		DialogueBottomChrome->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UCanvasPanelSlot* ChromeSlot = Cast<UCanvasPanelSlot>(DialogueBottomChrome->Slot))
		{
			ChromeSlot->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f));
			ChromeSlot->SetAlignment(FVector2D(0.f, 1.f));
			ChromeSlot->SetAutoSize(false);
			ChromeSlot->SetOffsets(FMargin(
				DialogueSideMargin + DialoguePortraitSize + DialogueGap,
				-DialogueBottomMargin,
				DialogueSideMargin,
				DialoguePortraitSize));
			ChromeSlot->SetZOrder(10);
		}
	}

	if (WidgetTree)
	{
		if (UBorder* DialogueBox = Cast<UBorder>(WidgetTree->FindWidget(TEXT("DialogueBox"))))
		{
			ApplySoftPanelBrush(DialogueBox, DialogBoxBg);
			DialogueBox->SetPadding(FMargin(28.f, 24.f, 28.f, 18.f));
			DialogueBox->SetVisibility(ESlateVisibility::HitTestInvisible);
			if (UCanvasPanelSlot* BoxSlot = Cast<UCanvasPanelSlot>(DialogueBox->Slot))
			{
				BoxSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
				BoxSlot->SetAutoSize(false);
				BoxSlot->SetOffsets(FMargin(0.f));
			}
		}

		if (UTextBlock* ContinueHint = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ContinueHint"))))
		{
			ContinueHint->SetText(FText::FromString(TEXT("Click / Space to continue")));
			ContinueHint->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.35f, 0.4f, 1.f)));
			ContinueHint->SetJustification(ETextJustify::Right);
			ContinueHint->SetVisibility(ESlateVisibility::HitTestInvisible);
			ApplyHudFont(ContinueHint, FontBody);

			// Always pin to dialogue box bottom-right.
			if (DialogueBottomChrome)
			{
				ContinueHint->RemoveFromParent();
				UCanvasPanelSlot* HintCSlot = DialogueBottomChrome->AddChildToCanvas(ContinueHint);
				HintCSlot->SetAnchors(FAnchors(1.f, 1.f));
				HintCSlot->SetAlignment(FVector2D(1.f, 1.f));
				HintCSlot->SetAutoSize(true);
				HintCSlot->SetPosition(FVector2D(-28.f, -18.f));
				HintCSlot->SetZOrder(12);
			}
		}
	}

	if (DialogueNameText)
	{
		DialogueNameText->SetColorAndOpacity(FSlateColor(DarkText));
		DialogueNameText->SetJustification(ETextJustify::Center);
		DialogueNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
		ApplyHudFont(DialogueNameText, FontDialogueName);
	}
	if (DialogueBodyText)
	{
		DialogueBodyText->SetColorAndOpacity(FSlateColor(DarkText));
		DialogueBodyText->SetAutoWrapText(true);
		DialogueBodyText->SetVisibility(ESlateVisibility::HitTestInvisible);
		ApplyHudFont(DialogueBodyText, FontDialogueBody);
	}
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

	EnsureHudStyleAssets();

	// Full-screen dim; centered storybook modal card.
	ResultPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResultPanel"));
	ResultPanel->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.35f));
	ResultPanel->SetPadding(FMargin(0.f));
	ResultPanel->SetVisibility(ESlateVisibility::Collapsed);

	UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(ResultPanel);
	PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	PanelSlot->SetOffsets(FMargin(0.f));
	PanelSlot->SetZOrder(60);

	UCanvasPanel* Inner = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ResultInner"));
	ResultPanel->SetContent(Inner);

	USizeBox* CardWidth = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ResultCardWidth"));
	CardWidth->SetWidthOverride(460.f);
	{
		UCanvasPanelSlot* CardSlot = Inner->AddChildToCanvas(CardWidth);
		CardSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CardSlot->SetAutoSize(true);
	}

	ResultCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResultCard"));
	ApplySoftPanelBrush(ResultCard, ModalCardBg);
	ResultCard->SetPadding(FMargin(28.f, 24.f));
	CardWidth->SetContent(ResultCard);

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ResultCol"));
	ResultCard->SetContent(Col);

	ResultTitlePlate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResultTitlePlate"));
	ApplySoftPanelBrush(ResultTitlePlate, NamePlateBg);
	ResultTitlePlate->SetPadding(FMargin(20.f, 8.f));
	{
		UVerticalBoxSlot* PlateSlot = Col->AddChildToVerticalBox(ResultTitlePlate);
		PlateSlot->SetHorizontalAlignment(HAlign_Center);
		PlateSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
	}

	ResultTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResultTitleText"));
	ResultTitleText->SetText(FText::FromString(TEXT("Defeat")));
	ResultTitleText->SetJustification(ETextJustify::Center);
	ResultTitleText->SetColorAndOpacity(FSlateColor(DarkText));
	ApplyHudFont(ResultTitleText, FontModalTitle);
	ResultTitlePlate->SetContent(ResultTitleText);

	ResultBodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResultBodyText"));
	ResultBodyText->SetText(FText::FromString(TEXT("")));
	ResultBodyText->SetJustification(ETextJustify::Center);
	ResultBodyText->SetColorAndOpacity(FSlateColor(DarkText));
	ResultBodyText->SetAutoWrapText(true);
	ApplyHudFont(ResultBodyText, FontBody);
	{
		UVerticalBoxSlot* BodySlot = Col->AddChildToVerticalBox(ResultBodyText);
		BodySlot->SetHorizontalAlignment(HAlign_Fill);
		BodySlot->SetPadding(FMargin(8.f, 0.f, 8.f, 22.f));
	}

	UHorizontalBox* BtnRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ResultBtnRow"));
	{
		UVerticalBoxSlot* RowSlot = Col->AddChildToVerticalBox(BtnRow);
		RowSlot->SetHorizontalAlignment(HAlign_Center);
	}

	auto MakeResultBtn = [&](UButton*& OutBtn, UTextBlock*& OutLabel, const TCHAR* Label, FName Name,
		const FLinearColor& Normal, const FLinearColor& Hover) -> void
	{
		USizeBox* BtnBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		BtnBox->SetMinDesiredHeight(40.f);
		BtnBox->SetWidthOverride(140.f);

		OutBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		OutLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		OutLabel->SetText(FText::FromString(Label));
		OutLabel->SetJustification(ETextJustify::Center);
		OutLabel->SetColorAndOpacity(FSlateColor(CostOnDark));
		ApplyHudFont(OutLabel, FontBody);
		OutBtn->AddChild(OutLabel);
		ApplySoftButtonStyle(OutBtn, Normal, Hover, CardSelectedBg);
		BtnBox->SetContent(OutBtn);

		UHorizontalBoxSlot* Added = BtnRow->AddChildToHorizontalBox(BtnBox);
		Added->SetPadding(FMargin(8.f, 0.f));
		Added->SetVerticalAlignment(VAlign_Center);
	};

	MakeResultBtn(BtnPrimaryResult, PrimaryResultLabel, TEXT("Replay"), TEXT("BtnPrimaryResult"),
		DispatchBtnBg, DispatchBtnHover);
	MakeResultBtn(BtnQuit, QuitResultLabel, TEXT("Quit"), TEXT("BtnQuit"),
		QuitBtnBg, QuitBtnHover);
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
	if (ResourceBarPanel)
	{
		ResourceBarPanel->SetVisibility(Vis);
	}
	for (UTextBlock* Label : WorldLabelPool)
	{
		if (Label)
		{
			Label->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
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

	EnsureHudStyleAssets();

	UpgradeToastPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("UpgradeToastPanel"));
	ApplySoftPanelBrush(UpgradeToastPanel, ModalCardBg);
	UpgradeToastPanel->SetPadding(FMargin(18.f, 14.f));
	UpgradeToastPanel->SetVisibility(ESlateVisibility::Collapsed);
	UpgradeToastPanel->SetRenderOpacity(1.f);

	UpgradeToastSlot = Canvas->AddChildToCanvas(UpgradeToastPanel);
	UpgradeToastSlot->SetAnchors(FAnchors(0.5f, 0.f));
	UpgradeToastSlot->SetAlignment(FVector2D(0.5f, 0.f));
	UpgradeToastSlot->SetAutoSize(true);
	UpgradeToastSlot->SetPosition(FVector2D(0.f, -140.f));
	UpgradeToastSlot->SetZOrder(55);

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("UpgradeToastCol"));
	UpgradeToastPanel->SetContent(Col);

	UpgradeToastTitlePlate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("UpgradeToastTitlePlate"));
	ApplySoftPanelBrush(UpgradeToastTitlePlate, NamePlateBg);
	UpgradeToastTitlePlate->SetPadding(FMargin(16.f, 6.f));
	{
		UVerticalBoxSlot* PlateSlot = Col->AddChildToVerticalBox(UpgradeToastTitlePlate);
		PlateSlot->SetHorizontalAlignment(HAlign_Center);
		PlateSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}

	UpgradeToastTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UpgradeToastTitle"));
	UpgradeToastTitleText->SetText(FText::FromString(TEXT("Upgraded")));
	UpgradeToastTitleText->SetColorAndOpacity(FSlateColor(DarkText));
	UpgradeToastTitleText->SetJustification(ETextJustify::Center);
	ApplyHudFont(UpgradeToastTitleText, FontTitle);
	UpgradeToastTitlePlate->SetContent(UpgradeToastTitleText);

	UpgradeToastBodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UpgradeToastBody"));
	UpgradeToastBodyText->SetText(FText::FromString(TEXT("")));
	UpgradeToastBodyText->SetColorAndOpacity(FSlateColor(DarkText));
	UpgradeToastBodyText->SetJustification(ETextJustify::Center);
	ApplyHudFont(UpgradeToastBodyText, FontBody);
	{
		UVerticalBoxSlot* BodySlot = Col->AddChildToVerticalBox(UpgradeToastBodyText);
		BodySlot->SetHorizontalAlignment(HAlign_Center);
	}
}

void URTSGameHUD::RefreshUpgradeToastRestY()
{
	constexpr float GapBelowWave = 12.f;
	constexpr float FallbackRestY = 78.f;

	UpgradeToastRestY = FallbackRestY;
	if (!WavePanel)
	{
		return;
	}

	float WaveTop = 12.f;
	if (UCanvasPanelSlot* WaveSlot = Cast<UCanvasPanelSlot>(WavePanel->Slot))
	{
		WaveTop = WaveSlot->GetPosition().Y;
	}

	float WaveH = WavePanel->GetDesiredSize().Y;
	if (WaveH < 1.f)
	{
		WaveH = WavePanel->GetCachedGeometry().GetLocalSize().Y;
	}
	if (WaveH < 1.f)
	{
		WaveH = 48.f;
	}

	UpgradeToastRestY = WaveTop + WaveH + GapBelowWave;
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
	UpgradeToastTitleText->SetColorAndOpacity(FSlateColor(DarkText));
	ApplyHudFont(UpgradeToastTitleText, FontTitle);
	UpgradeToastBodyText->SetText(FText::FromString(
		FString::Printf(TEXT("Bonus: %s"), *Bonus)));
	UpgradeToastBodyText->SetColorAndOpacity(FSlateColor(DarkText));
	ApplyHudFont(UpgradeToastBodyText, FontBody);
	if (UpgradeToastPanel)
	{
		ApplySoftPanelBrush(UpgradeToastPanel, ModalCardBg);
	}
	if (UpgradeToastTitlePlate)
	{
		ApplySoftPanelBrush(UpgradeToastTitlePlate, NamePlateBg);
	}

	RefreshUpgradeToastRestY();

	UpgradeToastAge = 0.f;
	bUpgradeToastActive = true;
	UpgradeToastPanel->SetRenderOpacity(1.f);
	UpgradeToastPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UpgradeToastSlot)
	{
		UpgradeToastSlot->SetPosition(FVector2D(0.f, UpgradeToastRestY - 160.f));
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

	const float StartY = UpgradeToastRestY - 160.f;
	if (UpgradeToastAge < DropDuration)
	{
		const float T = UpgradeToastAge / DropDuration;
		const float Ease = 1.f - FMath::Square(1.f - T); // ease-out
		Y = FMath::Lerp(StartY, UpgradeToastRestY, Ease);
	}
	else if (UpgradeToastAge > DropDuration + HoldDuration)
	{
		const float FadeT = (UpgradeToastAge - DropDuration - HoldDuration) / FadeDuration;
		Opacity = 1.f - FMath::Clamp(FadeT, 0.f, 1.f);
		Y = UpgradeToastRestY + FadeT * 12.f; // drift slightly down while fading
	}

	UpgradeToastSlot->SetPosition(FVector2D(0.f, Y));
	UpgradeToastPanel->SetRenderOpacity(Opacity);
}

void URTSGameHUD::CollectTextBlocks(UWidget* Root, TArray<UTextBlock*>& OutTexts)
{
	if (!Root)
	{
		return;
	}
	if (UTextBlock* AsText = Cast<UTextBlock>(Root))
	{
		OutTexts.Add(AsText);
		return;
	}
	if (UPanelWidget* Panel = Cast<UPanelWidget>(Root))
	{
		const int32 Num = Panel->GetChildrenCount();
		for (int32 i = 0; i < Num; ++i)
		{
			CollectTextBlocks(Panel->GetChildAt(i), OutTexts);
		}
	}
}

void URTSGameHUD::ResolveDesignerBindings()
{
	if (!WidgetTree)
	{
		return;
	}

	auto FindBtn = [this](UButton*& OutBtn, const TCHAR* Name)
	{
		if (!OutBtn)
		{
			OutBtn = Cast<UButton>(WidgetTree->FindWidget(Name));
		}
	};
	auto FindText = [this](UTextBlock*& OutText, const TCHAR* Name)
	{
		if (!OutText)
		{
			OutText = Cast<UTextBlock>(WidgetTree->FindWidget(Name));
		}
	};

	FindBtn(BtnRabbit, TEXT("BtnRabbit"));
	FindBtn(BtnChicken, TEXT("BtnChicken"));
	FindBtn(BtnSheep, TEXT("BtnSheep"));
	FindBtn(BtnPig, TEXT("BtnPig"));
	FindBtn(BtnUpgradeRabbit, TEXT("BtnUpgradeRabbit"));
	FindBtn(BtnUpgradeChicken, TEXT("BtnUpgradeChicken"));
	FindBtn(BtnUpgradeSheep, TEXT("BtnUpgradeSheep"));
	FindBtn(BtnUpgradePig, TEXT("BtnUpgradePig"));

	FindText(FodderText, TEXT("FodderText"));
	FindText(SoulText, TEXT("SoulText"));
	FindText(WaveText, TEXT("WaveText"));
	FindText(ObjectiveText, TEXT("ObjectiveText"));
	FindText(StatusText, TEXT("StatusText"));
	FindText(CostRabbitText, TEXT("CostRabbitText"));
	FindText(CostChickenText, TEXT("CostChickenText"));
	FindText(CostSheepText, TEXT("CostSheepText"));
	FindText(CostPigText, TEXT("CostPigText"));
	FindText(UpgradeRabbitText, TEXT("UpgradeRabbitText"));
	FindText(UpgradeChickenText, TEXT("UpgradeChickenText"));
	FindText(UpgradeSheepText, TEXT("UpgradeSheepText"));
	FindText(UpgradePigText, TEXT("UpgradePigText"));

	// Dialogue (WBP_RTSGameHUD BindWidget names)
	if (!DialoguePanel) { DialoguePanel = Cast<UBorder>(WidgetTree->FindWidget(TEXT("DialoguePanel"))); }
	if (!DialogueNameText) { DialogueNameText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("DialogueNameText"))); }
	if (!DialogueBodyText) { DialogueBodyText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("DialogueBodyText"))); }
	FindBtn(BtnDialogueAdvance, TEXT("BtnDialogueAdvance"));
	if (!PortraitBlock) { PortraitBlock = Cast<UBorder>(WidgetTree->FindWidget(TEXT("PortraitBlock"))); }
	if (!NamePlate) { NamePlate = Cast<UBorder>(WidgetTree->FindWidget(TEXT("NamePlate"))); }
	if (!DialogueBottomChrome) { DialogueBottomChrome = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("DialogueBottomChrome"))); }
	if (!DialoguePortrait) { DialoguePortrait = Cast<UImage>(WidgetTree->FindWidget(TEXT("DialoguePortrait"))); }

	// Re-bind clicks in case buttons were resolved after the first bind pass.
	BindFallbackButtonHandlers();
}

void URTSGameHUD::RefreshRecruitSlot(UButton* Button, UTextBlock* CostText, int32 Cost, int32 AvailableFodder)
{
	if (CostText)
	{
		CostText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Cost)));
		CostText->SetColorAndOpacity(FSlateColor(CostOnDark));
	}
	if (Button)
	{
		Button->SetIsEnabled(AvailableFodder >= Cost);
	}
}

void URTSGameHUD::RefreshUpgradeSlot(UButton* Button, UTextBlock*& LevelText, UTextBlock*& CostText, UImage* CostIcon, ERTSUnitType Type, ARTSGameMode* GM)
{
	if (!GM)
	{
		return;
	}

	const int32 Lv = GM->GetUnitUpgradeLevel(Type);
	const bool bMax = Lv >= 3;
	if (LevelText)
	{
		LevelText->SetText(FText::FromString(FString::Printf(TEXT("Lv%d"), bMax ? 3 : Lv)));
		LevelText->SetColorAndOpacity(FSlateColor(DarkText));
	}

	const int32 NextCost = bMax ? 0 : GM->GetUpgradeCost(Lv + 1);
	if (CostText)
	{
		CostText->SetText(bMax ? FText::FromString(TEXT("MAX")) : FText::FromString(FString::Printf(TEXT("%d"), NextCost)));
		CostText->SetColorAndOpacity(FSlateColor(CostOnDark));
		CostText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (CostIcon)
	{
		CostIcon->SetVisibility(bMax ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (Button)
	{
		Button->SetIsEnabled(!bMax && GM->Soul >= NextCost);
		Button->SetVisibility(ESlateVisibility::Visible);
	}
}

void URTSGameHUD::EnsureHudStyleAssets()
{
	if (!HudFont)
	{
		HudFont = LoadObject<UFont>(nullptr, TEXT("/Game/Fonts/Galdeano.Galdeano"));
		if (!HudFont)
		{
			HudFont = LoadObject<UFont>(nullptr, TEXT("/Game/Fonts/Galdeanofont.Galdeanofont"));
		}
	}

	if (!RoundPanelTexture)
	{
		RoundPanelTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/T_UI_RoundPanel.T_UI_RoundPanel"));
		if (!RoundPanelTexture)
		{
			RoundPanelTexture = CreateRoundPanelTexture();
		}
	}
}

void URTSGameHUD::ApplyHudFont(UTextBlock* Text, int32 Size) const
{
	if (!Text || !HudFont)
	{
		return;
	}
	FSlateFontInfo Info = Text->Font;
	Info.FontObject = HudFont;
	Info.Size = Size;
	Text->SetFont(Info);
}

UTexture2D* URTSGameHUD::CreateRoundPanelTexture()
{
	constexpr int32 Size = 64;
	constexpr float Radius = 14.f;

	UTexture2D* Tex = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8);
	if (!Tex || !Tex->PlatformData || Tex->PlatformData->Mips.Num() == 0)
	{
		return nullptr;
	}

	Tex->SRGB = true;
	Tex->Filter = TF_Bilinear;
	Tex->CompressionSettings = TC_Default;
	Tex->AddressX = TA_Clamp;
	Tex->AddressY = TA_Clamp;

	FTexture2DMipMap& Mip = Tex->PlatformData->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FColor* Pixels = static_cast<FColor*>(Data);

	auto InsideRounded = [&](int32 X, int32 Y) -> bool
	{
		const float Px = X + 0.5f;
		const float Py = Y + 0.5f;
		float Cx = -1.f;
		float Cy = -1.f;
		if (Px < Radius && Py < Radius)
		{
			Cx = Radius;
			Cy = Radius;
		}
		else if (Px >= Size - Radius && Py < Radius)
		{
			Cx = Size - Radius;
			Cy = Radius;
		}
		else if (Px < Radius && Py >= Size - Radius)
		{
			Cx = Radius;
			Cy = Size - Radius;
		}
		else if (Px >= Size - Radius && Py >= Size - Radius)
		{
			Cx = Size - Radius;
			Cy = Size - Radius;
		}
		else
		{
			return true;
		}
		const float Dx = Px - Cx;
		const float Dy = Py - Cy;
		return (Dx * Dx + Dy * Dy) <= (Radius * Radius);
	};

	for (int32 Y = 0; Y < Size; ++Y)
	{
		for (int32 X = 0; X < Size; ++X)
		{
			Pixels[Y * Size + X] = InsideRounded(X, Y)
				? FColor(255, 255, 255, 255)
				: FColor(0, 0, 0, 0);
		}
	}

	Mip.BulkData.Unlock();
	Tex->UpdateResource();
	return Tex;
}

void URTSGameHUD::ApplySoftPanelBrush(UBorder* Border, const FLinearColor& Tint)
{
	if (!Border)
	{
		return;
	}
	EnsureHudStyleAssets();

	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::Box;
	Brush.TintColor = FSlateColor(Tint);
	Brush.Margin = FMargin(0.4f);
	Brush.ImageSize = FVector2D(64.f, 64.f);
	if (RoundPanelTexture)
	{
		Brush.SetResourceObject(RoundPanelTexture);
	}
	else
	{
		const FButtonStyle& Ref = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
		Brush = Ref.Normal;
		Brush.TintColor = FSlateColor(Tint);
	}
	Border->SetBrush(Brush);
}

void URTSGameHUD::ApplySoftButtonStyle(UButton* Btn, const FLinearColor& Normal, const FLinearColor& Hovered, const FLinearColor& Pressed)
{
	if (!Btn)
	{
		return;
	}
	EnsureHudStyleAssets();

	auto MakeBrush = [this](const FLinearColor& Tint) -> FSlateBrush
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.TintColor = FSlateColor(Tint);
		Brush.Margin = FMargin(0.4f);
		Brush.ImageSize = FVector2D(64.f, 64.f);
		if (RoundPanelTexture)
		{
			Brush.SetResourceObject(RoundPanelTexture);
		}
		return Brush;
	};

	FButtonStyle Style = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
	if (RoundPanelTexture)
	{
		Style.Normal = MakeBrush(Normal);
		Style.Hovered = MakeBrush(Hovered);
		Style.Pressed = MakeBrush(Pressed);
		Style.Disabled = MakeBrush(FLinearColor(Normal.R, Normal.G, Normal.B, 0.4f));
	}
	else
	{
		Style.Normal.TintColor = FSlateColor(Normal);
		Style.Hovered.TintColor = FSlateColor(Hovered);
		Style.Pressed.TintColor = FSlateColor(Pressed);
		Style.Disabled.TintColor = FSlateColor(FLinearColor(Normal.R, Normal.G, Normal.B, 0.35f));
	}
	Style.NormalPadding = FMargin(10.f, 9.f);
	Style.PressedPadding = FMargin(10.f, 9.f);
	Btn->SetStyle(Style);
}

void URTSGameHUD::EnsureWorldLabelLayer()
{
	// Labels are created on demand in UpdateWorldLabels / AcquireWorldLabel.
	EnsureHudStyleAssets();
}

UTextBlock* URTSGameHUD::AcquireWorldLabel(int32 Index, const FString& Text, const FLinearColor& Color)
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return nullptr;
	}

	while (WorldLabelPool.Num() <= Index)
	{
		const int32 NewIndex = WorldLabelPool.Num();
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("WorldLabel_%d"), NewIndex));
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		Label->SetJustification(ETextJustify::Center);
		ApplyHudFont(Label, FontWorldLabel);

		UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(Label);
		CanvasSlot->SetAnchors(FAnchors(0.f, 0.f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetZOrder(4);

		WorldLabelPool.Add(Label);
	}

	UTextBlock* Label = WorldLabelPool[Index];
	if (!Label)
	{
		return nullptr;
	}

	Label->SetText(FText::FromString(Text));
	Label->SetColorAndOpacity(FSlateColor(Color));
	ApplyHudFont(Label, FontWorldLabel);
	Label->SetVisibility(ESlateVisibility::HitTestInvisible);
	return Label;
}

void URTSGameHUD::UpdateWorldLabels()
{
	EnsureWorldLabelLayer();

	const bool bHide = bDialogueActive || bResultShown;
	if (bHide)
	{
		for (UTextBlock* Label : WorldLabelPool)
		{
			if (Label)
			{
				Label->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	UWorld* World = GetWorld();
	if (!PC || !World)
	{
		return;
	}

	int32 LabelIndex = 0;

	auto PlaceLabel = [&](const FVector& WorldLoc, const FString& Text, const FLinearColor& Color)
	{
		// Screen pixels != UMG local coords (DPI / viewport). Convert via WidgetLayoutLibrary.
		FVector2D WidgetPos;
		if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, WorldLoc, WidgetPos, true))
		{
			return;
		}

		UTextBlock* Label = AcquireWorldLabel(LabelIndex, Text, Color);
		++LabelIndex;
		if (!Label)
		{
			return;
		}

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Label->Slot))
		{
			CanvasSlot->SetPosition(WidgetPos);
		}
	};

	TArray<AActor*> Buildings;
	UGameplayStatics::GetAllActorsOfClass(World, ARTSBaseBuilding::StaticClass(), Buildings);
	for (AActor* Actor : Buildings)
	{
		ARTSBaseBuilding* Building = Cast<ARTSBaseBuilding>(Actor);
		if (!Building || !Building->IsAlive() || !Building->bIsCoreBuilding)
		{
			continue;
		}
		PlaceLabel(
			Building->GetActorLocation() + FVector(0.f, 0.f, 240.f),
			TEXT("Chicken Coop"),
			FLinearColor(1.f, 0.9f, 0.35f, 1.f));
	}

	TArray<AActor*> Resources;
	UGameplayStatics::GetAllActorsOfClass(World, ARTSResourceNode::StaticClass(), Resources);
	for (AActor* Actor : Resources)
	{
		if (!Actor)
		{
			continue;
		}
		PlaceLabel(
			Actor->GetActorLocation() + FVector(0.f, 0.f, 120.f),
			TEXT("Fodder Point"),
			FLinearColor(0.4f, 1.f, 0.45f, 1.f));
	}

	for (int32 i = LabelIndex; i < WorldLabelPool.Num(); ++i)
	{
		if (WorldLabelPool[i])
		{
			WorldLabelPool[i]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void URTSGameHUD::ApplyDesignerLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	auto ResolveBorder = [this](UBorder*& OutBorder, const TCHAR* Name)
	{
		if (!OutBorder)
		{
			OutBorder = Cast<UBorder>(WidgetTree->FindWidget(Name));
		}
	};
	auto ResolveSizeBox = [this](USizeBox*& OutBox, const TCHAR* Name)
	{
		if (!OutBox)
		{
			OutBox = Cast<USizeBox>(WidgetTree->FindWidget(Name));
		}
	};

	ResolveBorder(WavePanel, TEXT("WavePanel"));
	ResolveBorder(ObjectivePanel, TEXT("ObjectivePanel"));
	ResolveSizeBox(SideBarsBox, TEXT("SideBarsBox"));

	if (WavePanel)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(WavePanel->Slot))
		{
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.f, 0.5f, 0.f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.f));
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetPosition(FVector2D(0.f, 12.f));
			CanvasSlot->SetZOrder(5);
		}
	}

	if (ObjectivePanel)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ObjectivePanel->Slot))
		{
			CanvasSlot->SetAnchors(FAnchors(0.f, 0.f));
			CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetPosition(FVector2D(12.f, 12.f));
			CanvasSlot->SetZOrder(5);
		}
	}

	if (ResourceBarPanel)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ResourceBarPanel->Slot))
		{
			CanvasSlot->SetAnchors(FAnchors(0.f, 0.f));
			CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetPosition(FVector2D(12.f, 56.f));
			CanvasSlot->SetZOrder(5);
		}
	}

	if (SideBarsBox)
	{
		SideBarsBox->SetWidthOverride(SideBarWidth);
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SideBarsBox->Slot))
		{
			CanvasSlot->SetAnchors(FAnchors(0.f, 0.f));
			CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetPosition(FVector2D(12.f, 100.f));
			CanvasSlot->SetZOrder(5);
		}
	}
}

UBorder* URTSGameHUD::WrapInColoredBorder(UWidget* Child, const FLinearColor& Color, const FMargin& InPadding, FName Name)
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
	ApplySoftPanelBrush(Border, Color);
	Border->SetPadding(InPadding);
	if (Child)
	{
		Border->SetContent(Child);
	}
	return Border;
}

void URTSGameHUD::ApplyDesignerChrome()
{
	if (!WidgetTree)
	{
		return;
	}

	EnsureHudStyleAssets();
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);

	// --- Wave bar (top center) ---
	if (WavePanel)
	{
		ApplySoftPanelBrush(WavePanel, InfoBarBg);
		WavePanel->SetPadding(FMargin(20.f, 8.f));

		TArray<UTextBlock*> WaveTexts;
		CollectTextBlocks(WavePanel, WaveTexts);
		if (!WaveText && WaveTexts.Num() > 0)
		{
			WaveText = WaveTexts[0];
		}
		if (WaveTexts.Num() > 1 && !StatusText)
		{
			StatusText = WaveTexts[1];
		}
		for (UTextBlock* T : WaveTexts)
		{
			if (T)
			{
				T->SetColorAndOpacity(FSlateColor(DarkText));
				T->SetJustification(ETextJustify::Center);
				ApplyHudFont(T, FontTitle);
			}
		}
		if (WaveText)
		{
			WaveText->SetText(FText::FromString(TEXT("Wave -")));
		}
	}

	// --- Objective (top left) ---
	if (ObjectivePanel)
	{
		ApplySoftPanelBrush(ObjectivePanel, InfoBarBg);
		ObjectivePanel->SetPadding(FMargin(12.f, 8.f));

		TArray<UTextBlock*> ObjTexts;
		CollectTextBlocks(ObjectivePanel, ObjTexts);
		if (!ObjectiveText && ObjTexts.Num() > 0)
		{
			ObjectiveText = ObjTexts[0];
		}
		if (ObjectiveText)
		{
			ObjectiveText->SetText(FText::FromString(TEXT("Objective: Protect the Chicken Coop")));
			ObjectiveText->SetColorAndOpacity(FSlateColor(DarkText));
			ObjectiveText->SetAutoWrapText(true);
			ApplyHudFont(ObjectiveText, FontTitle);
		}
	}

	// --- Sidebar: single column of unit cards ---
	UVerticalBox* Sidebar = nullptr;
	if (SideBarsBox && SideBarsBox->GetChildrenCount() > 0)
	{
		Sidebar = Cast<UVerticalBox>(SideBarsBox->GetChildAt(0));
	}
	if (!Sidebar)
	{
		return;
	}

	TArray<UWidget*> Kids;
	Kids.Reserve(Sidebar->GetChildrenCount());
	for (int32 i = 0; i < Sidebar->GetChildrenCount(); ++i)
	{
		Kids.Add(Sidebar->GetChildAt(i));
	}
	Sidebar->ClearChildren();

	auto Take = [&](UWidget* W) -> UWidget*
	{
		return Kids.Contains(W) ? W : nullptr;
	};

	// Merged resource bar under Objective (fodder + soul).
	if (RootCanvas)
	{
		if (ResourceBarPanel)
		{
			if (UWidget* Parent = ResourceBarPanel->GetParent())
			{
				Parent->RemoveFromParent();
			}
			else
			{
				ResourceBarPanel->RemoveFromParent();
			}
		}
		ResourceBarPanel = WrapInColoredBorder(nullptr, InfoBarBg, FMargin(12.f, 6.f), TEXT("ResourceBarChrome"));
		UHorizontalBox* ResRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ResourceRowChrome"));
		ResourceBarPanel->SetContent(ResRow);

		auto AddRes = [&](UTextBlock*& Text, const TCHAR* FallbackName, const TCHAR* DefaultLabel)
		{
			if (!Text)
			{
				Text = Cast<UTextBlock>(WidgetTree->FindWidget(FallbackName));
			}
			if (!Text)
			{
				Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FallbackName);
			}
			Take(Text);
			Text->RemoveFromParent();
			Text->SetColorAndOpacity(FSlateColor(DarkText));
			Text->SetText(FText::FromString(DefaultLabel));
			ApplyHudFont(Text, FontBody);
			UHorizontalBoxSlot* Slot = ResRow->AddChildToHorizontalBox(Text);
			Slot->SetVerticalAlignment(VAlign_Center);
			Slot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
		};
		AddRes(FodderText, TEXT("FodderText"), TEXT("Fodder 0"));
		AddRes(SoulText, TEXT("SoulText"), TEXT("Souls 0"));

		USizeBox* ResWidthBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ResourceBarWidth"));
		ResWidthBox->SetWidthOverride(SideBarWidth);
		ResWidthBox->SetContent(ResourceBarPanel);

		UCanvasPanelSlot* ResSlot = RootCanvas->AddChildToCanvas(ResWidthBox);
		ResSlot->SetAnchors(FAnchors(0.f, 0.f));
		ResSlot->SetAlignment(FVector2D(0.f, 0.f));
		ResSlot->SetAutoSize(true);
		ResSlot->SetPosition(FVector2D(12.f, 56.f));
		ResSlot->SetZOrder(5);
	}

	auto MakeActionButtonContent = [&](UButton* Btn, const TCHAR* Label, UTextBlock*& CostText, UImage*& CostIcon, const TCHAR* IconPath, FName CostName)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		LabelText->SetText(FText::FromString(Label));
		LabelText->SetColorAndOpacity(FSlateColor(CostOnDark));
		ApplyHudFont(LabelText, FontSmall);
		{
			UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText);
			LabelSlot->SetVerticalAlignment(VAlign_Center);
			LabelSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
		}

		if (!CostText)
		{
			CostText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), CostName);
		}
		CostText->RemoveFromParent();
		CostText->SetColorAndOpacity(FSlateColor(CostOnDark));
		CostText->SetText(FText::FromString(TEXT("0")));
		ApplyHudFont(CostText, FontSmall);
		{
			UHorizontalBoxSlot* CostSlot = Row->AddChildToHorizontalBox(CostText);
			CostSlot->SetVerticalAlignment(VAlign_Center);
			CostSlot->SetPadding(FMargin(0.f, 0.f, 4.f, 0.f));
		}

		if (!CostIcon)
		{
			CostIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		}
		CostIcon->RemoveFromParent();
		SetImageFromPath(CostIcon, IconPath, 16.f);
		{
			UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(CostIcon);
			IconSlot->SetVerticalAlignment(VAlign_Center);
		}

		Btn->SetContent(Row);
		if (UButtonSlot* BtnSlot = Cast<UButtonSlot>(Row->Slot))
		{
			BtnSlot->SetHorizontalAlignment(HAlign_Center);
			BtnSlot->SetVerticalAlignment(VAlign_Center);
			BtnSlot->SetPadding(FMargin(2.f, 1.f));
		}
	};

	auto BuildUnitCard = [&](
		UBorder*& CardBorder,
		UButton* DispatchBtn,
		UButton* UpgradeBtn,
		UTextBlock*& LevelText,
		UTextBlock*& DispatchCostText,
		UTextBlock*& UpgradeCostText,
		UImage*& UnitIcon,
		UImage*& FodderCostIcon,
		UImage*& SoulCostIcon,
		const TCHAR* UnitName,
		const TCHAR* IconPath,
		FName CardName)
	{
		if (!DispatchBtn || !UpgradeBtn)
		{
			return;
		}
		Take(DispatchBtn);
		Take(UpgradeBtn);
		if (LevelText)
		{
			Take(LevelText);
		}

		if (!UnitIcon)
		{
			UnitIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		}
		UnitIcon->RemoveFromParent();
		const float InnerIconSize = RecruitIconSize - 10.f;
		SetImageFromPath(UnitIcon, IconPath, InnerIconSize);

		UBorder* IconBadge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		ApplySoftPanelBrush(IconBadge, IconBadgeBg);
		IconBadge->SetPadding(FMargin(5.f));
		IconBadge->SetContent(UnitIcon);

		USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		IconBox->SetWidthOverride(RecruitIconSize);
		IconBox->SetHeightOverride(RecruitIconSize);
		IconBox->SetContent(IconBadge);

		UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		NameText->SetText(FText::FromString(UnitName));
		NameText->SetColorAndOpacity(FSlateColor(DarkText));
		NameText->SetJustification(ETextJustify::Center);
		ApplyHudFont(NameText, FontSmall);

		UVerticalBox* LeftCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		{
			UVerticalBoxSlot* IconSlot = LeftCol->AddChildToVerticalBox(IconBox);
			IconSlot->SetHorizontalAlignment(HAlign_Center);
		}
		{
			UVerticalBoxSlot* NameSlot = LeftCol->AddChildToVerticalBox(NameText);
			NameSlot->SetHorizontalAlignment(HAlign_Center);
			NameSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
		}

		if (!LevelText)
		{
			LevelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		}
		LevelText->RemoveFromParent();
		LevelText->SetText(FText::FromString(TEXT("Lv0")));
		LevelText->SetColorAndOpacity(FSlateColor(DarkText));
		ApplyHudFont(LevelText, FontBody);

		MakeActionButtonContent(UpgradeBtn, TEXT("Upgrade"), UpgradeCostText, SoulCostIcon, TEXT("/Game/UI/Icons/SoulIcon.SoulIcon"),
			*FString::Printf(TEXT("UpCost_%s"), UnitName));
		ApplySoftButtonStyle(UpgradeBtn, UpgradeBtnBg, UpgradeBtnHover, CardSelectedBg);

		MakeActionButtonContent(DispatchBtn, TEXT("Dispatch"), DispatchCostText, FodderCostIcon, TEXT("/Game/UI/Icons/FodderIcon.FodderIcon"),
			*FString::Printf(TEXT("DisCost_%s"), UnitName));
		ApplySoftButtonStyle(DispatchBtn, DispatchBtnBg, DispatchBtnHover, CardSelectedBg);

		auto WrapActionBtn = [&](UButton* Btn) -> USizeBox*
		{
			USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			Box->SetMinDesiredHeight(ActionBtnMinHeight);
			Box->SetWidthOverride(128.f);
			Btn->RemoveFromParent();
			Box->SetContent(Btn);
			return Box;
		};
		USizeBox* UpgradeBox = WrapActionBtn(UpgradeBtn);
		USizeBox* DispatchBox = WrapActionBtn(DispatchBtn);

		UVerticalBox* ActionCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		{
			UVerticalBoxSlot* UpSlot = ActionCol->AddChildToVerticalBox(UpgradeBox);
			UpSlot->SetHorizontalAlignment(HAlign_Fill);
			UpSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}
		{
			UVerticalBoxSlot* DisSlot = ActionCol->AddChildToVerticalBox(DispatchBox);
			DisSlot->SetHorizontalAlignment(HAlign_Fill);
		}

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		{
			UHorizontalBoxSlot* LeftSlot = Row->AddChildToHorizontalBox(LeftCol);
			LeftSlot->SetVerticalAlignment(VAlign_Center);
			LeftSlot->SetPadding(FMargin(2.f));
		}
		{
			UHorizontalBoxSlot* LvSlot = Row->AddChildToHorizontalBox(LevelText);
			LvSlot->SetVerticalAlignment(VAlign_Center);
			LvSlot->SetPadding(FMargin(6.f, 0.f, 6.f, 0.f));
			LvSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		{
			UHorizontalBoxSlot* ActSlot = Row->AddChildToHorizontalBox(ActionCol);
			ActSlot->SetVerticalAlignment(VAlign_Center);
			ActSlot->SetHorizontalAlignment(HAlign_Right);
			ActSlot->SetPadding(FMargin(2.f));
		}

		CardBorder = WrapInColoredBorder(Row, CardNormalBg, FMargin(4.f), CardName);
		UVerticalBoxSlot* CardSlot = Sidebar->AddChildToVerticalBox(CardBorder);
		CardSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		CardSlot->SetHorizontalAlignment(HAlign_Fill);
	};

	BuildUnitCard(UnitCardRabbit, BtnRabbit, BtnUpgradeRabbit, UpgradeRabbitText, CostRabbitText, UpgradeCostRabbitText,
		IconRabbit, IconFodderCostRabbit, IconSoulCostRabbit, TEXT("Rabbit"), TEXT("/Game/UI/Icons/T_Icon_Rabbit.T_Icon_Rabbit"), TEXT("UnitCardRabbit"));
	BuildUnitCard(UnitCardChicken, BtnChicken, BtnUpgradeChicken, UpgradeChickenText, CostChickenText, UpgradeCostChickenText,
		IconChicken, IconFodderCostChicken, IconSoulCostChicken, TEXT("Chicken"), TEXT("/Game/UI/Icons/T_Icon_Chicken.T_Icon_Chicken"), TEXT("UnitCardChicken"));
	BuildUnitCard(UnitCardSheep, BtnSheep, BtnUpgradeSheep, UpgradeSheepText, CostSheepText, UpgradeCostSheepText,
		IconSheep, IconFodderCostSheep, IconSoulCostSheep, TEXT("Sheep"), TEXT("/Game/UI/Icons/T_Icon_Sheep.T_Icon_Sheep"), TEXT("UnitCardSheep"));
	BuildUnitCard(UnitCardPig, BtnPig, BtnUpgradePig, UpgradePigText, CostPigText, UpgradeCostPigText,
		IconPig, IconFodderCostPig, IconSoulCostPig, TEXT("Pig"), TEXT("/Game/UI/Icons/T_Icon_Pig.T_Icon_Pig"), TEXT("UnitCardPig"));

	for (UWidget* W : Kids)
	{
		if (W && W->GetParent() == nullptr && W != FodderText && W != SoulText)
		{
			Sidebar->AddChildToVerticalBox(W);
		}
	}

	if (SideBarsBox)
	{
		SideBarsBox->SetWidthOverride(SideBarWidth);
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SideBarsBox->Slot))
		{
			CanvasSlot->SetPosition(FVector2D(12.f, 100.f));
		}
	}
}

void URTSGameHUD::SetImageFromPath(UImage* Image, const TCHAR* TexturePath, float BrushSize)
{
	if (!Image || !TexturePath)
	{
		return;
	}

	UTexture2D* Tex = LoadTextureFromPath(TexturePath);
	if (!Tex)
	{
		return;
	}

	Image->SetBrushFromTexture(Tex, true);
	Image->SetBrushSize(FVector2D(BrushSize, BrushSize));
	Image->SetVisibility(ESlateVisibility::HitTestInvisible);
}

UTexture2D* URTSGameHUD::LoadTextureFromPath(const TCHAR* TexturePath)
{
	return TexturePath ? LoadObject<UTexture2D>(nullptr, TexturePath) : nullptr;
}

UImage* URTSGameHUD::EnsureIconAfterWidget(UWidget* BeforeWidget, UImage*& IconSlot, FName IconName, const TCHAR* TexturePath, float BrushSize)
{
	if (!WidgetTree || !BeforeWidget)
	{
		return nullptr;
	}

	if (!IconSlot)
	{
		IconSlot = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), IconName);
	}
	SetImageFromPath(IconSlot, TexturePath, BrushSize);
	if (!IconSlot)
	{
		return nullptr;
	}

	// Already paired: parent is a tiny HBox that only holds text + this icon.
	if (UHorizontalBox* Pair = Cast<UHorizontalBox>(BeforeWidget->GetParent()))
	{
		if (Pair->GetChildIndex(IconSlot) != INDEX_NONE && Pair->GetChildrenCount() <= 3)
		{
			return IconSlot;
		}
	}

	UPanelWidget* Parent = Cast<UPanelWidget>(BeforeWidget->GetParent());
	if (!Parent)
	{
		return IconSlot;
	}

	// Wrap [BeforeWidget + Icon] as one unit so siblings (e.g. Up button) keep their layout.
	TArray<UWidget*> Order;
	for (int32 i = 0; i < Parent->GetChildrenCount(); ++i)
	{
		UWidget* Child = Parent->GetChildAt(i);
		if (Child && Child != IconSlot)
		{
			Order.Add(Child);
		}
	}

	UHorizontalBox* PairBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	BeforeWidget->RemoveFromParent();
	IconSlot->RemoveFromParent();
	{
		UHorizontalBoxSlot* TextSlot = PairBox->AddChildToHorizontalBox(BeforeWidget);
		TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		TextSlot->SetPadding(FMargin(0.f));
	}
	{
		UHorizontalBoxSlot* IconAdded = PairBox->AddChildToHorizontalBox(IconSlot);
		IconAdded->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));
		IconAdded->SetVerticalAlignment(VAlign_Center);
		IconAdded->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	if (UHorizontalBox* HParent = Cast<UHorizontalBox>(Parent))
	{
		HParent->ClearChildren();
		for (UWidget* Original : Order)
		{
			UWidget* W = (Original == BeforeWidget) ? static_cast<UWidget*>(PairBox) : Original;
			UHorizontalBoxSlot* Added = HParent->AddChildToHorizontalBox(W);
			Added->SetVerticalAlignment(VAlign_Center);
			if (W == PairBox)
			{
				Added->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				Added->SetPadding(FMargin(4.f, 2.f, 10.f, 2.f));
			}
			else if (Cast<UButton>(W))
			{
				Added->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
				Added->SetPadding(FMargin(10.f, 2.f, 4.f, 2.f));
				Added->SetHorizontalAlignment(HAlign_Right);
			}
			else if (Cast<UTextBlock>(W))
			{
				Added->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				Added->SetPadding(FMargin(6.f, 4.f));
			}
			else
			{
				Added->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
				Added->SetPadding(FMargin(6.f, 4.f));
			}
		}
		return IconSlot;
	}

	if (UBorder* BorderParent = Cast<UBorder>(Parent))
	{
		BorderParent->SetContent(PairBox);
		return IconSlot;
	}

	if (UVerticalBox* VParent = Cast<UVerticalBox>(Parent))
	{
		VParent->ClearChildren();
		for (UWidget* W : Order)
		{
			UWidget* ToAdd = (W == BeforeWidget) ? static_cast<UWidget*>(PairBox) : W;
			UVerticalBoxSlot* Added = VParent->AddChildToVerticalBox(ToAdd);
			Added->SetPadding(FMargin(0.f, 2.f));
			Added->SetHorizontalAlignment(HAlign_Right);
		}
		return IconSlot;
	}

	return IconSlot;
}

void URTSGameHUD::ApplyUnitIcons()
{
	// Resolve icons by BindWidget, or fall back to finding by name in the tree
	// (avoids needing Details panel to assign brushes in the WBP designer).
	auto Resolve = [this](UImage*& OutIcon, const TCHAR* WidgetName) -> UImage*
	{
		if (OutIcon)
		{
			return OutIcon;
		}
		if (WidgetTree)
		{
			if (UWidget* Found = WidgetTree->FindWidget(WidgetName))
			{
				OutIcon = Cast<UImage>(Found);
				return OutIcon;
			}
		}
		return nullptr;
	};

	SetImageFromPath(Resolve(IconRabbit, TEXT("IconRabbit")), TEXT("/Game/UI/Icons/T_Icon_Rabbit.T_Icon_Rabbit"), RecruitIconSize);
	SetImageFromPath(Resolve(IconChicken, TEXT("IconChicken")), TEXT("/Game/UI/Icons/T_Icon_Chicken.T_Icon_Chicken"), RecruitIconSize);
	SetImageFromPath(Resolve(IconSheep, TEXT("IconSheep")), TEXT("/Game/UI/Icons/T_Icon_Sheep.T_Icon_Sheep"), RecruitIconSize);
	SetImageFromPath(Resolve(IconPig, TEXT("IconPig")), TEXT("/Game/UI/Icons/T_Icon_Pig.T_Icon_Pig"), RecruitIconSize);
}

void URTSGameHUD::ApplyResourceIcons()
{
	constexpr float HeaderIconSize = 24.f;
	const TCHAR* SoulPath = TEXT("/Game/UI/Icons/SoulIcon.SoulIcon");
	const TCHAR* FodderPath = TEXT("/Game/UI/Icons/FodderIcon.FodderIcon");

	// Header totals only — per-card cost icons are built inside action buttons.
	EnsureIconAfterWidget(FodderText, IconFodderHeader, TEXT("IconFodderHeader"), FodderPath, HeaderIconSize);
	EnsureIconAfterWidget(SoulText, IconSoul, TEXT("IconSoul"), SoulPath, HeaderIconSize);
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

	ApplyDialogueDesignerLayout();
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
		DialogueNameText->SetColorAndOpacity(FSlateColor(DarkText));
		DialogueNameText->SetText(FText::FromString(
			DialogueSpeakers.IsValidIndex(DialogueLineIndex) ? DialogueSpeakers[DialogueLineIndex] : TEXT("?")));
	}
	if (DialogueBodyText)
	{
		DialogueBodyText->SetColorAndOpacity(FSlateColor(DarkText));
		DialogueBodyText->SetAutoWrapText(true);
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

	if (ResultCard)
	{
		ApplySoftPanelBrush(ResultCard, ModalCardBg);
	}
	if (ResultTitlePlate)
	{
		ApplySoftPanelBrush(ResultTitlePlate, NamePlateBg);
	}

	if (ResultTitleText)
	{
		ResultTitleText->SetText(FText::FromString(bVictory ? TEXT("Victory") : TEXT("Defeat")));
		ResultTitleText->SetColorAndOpacity(FSlateColor(DarkText));
		ApplyHudFont(ResultTitleText, FontModalTitle);
	}
	if (ResultBodyText)
	{
		ResultBodyText->SetText(FText::FromString(bVictory
			? TEXT("Fox-Wolf Coalition Repelled!")
			: TEXT("The chicken coop was destroyed.")));
		ResultBodyText->SetColorAndOpacity(FSlateColor(DarkText));
		ApplyHudFont(ResultBodyText, FontBody);
	}

	if (PrimaryResultLabel)
	{
		PrimaryResultLabel->SetText(FText::FromString(bVictory ? TEXT("Next Level") : TEXT("Replay")));
		PrimaryResultLabel->SetColorAndOpacity(FSlateColor(CostOnDark));
		ApplyHudFont(PrimaryResultLabel, FontBody);
	}
	if (QuitResultLabel)
	{
		QuitResultLabel->SetColorAndOpacity(FSlateColor(CostOnDark));
		ApplyHudFont(QuitResultLabel, FontBody);
	}
	ApplySoftButtonStyle(BtnPrimaryResult, DispatchBtnBg, DispatchBtnHover, CardSelectedBg);
	ApplySoftButtonStyle(BtnQuit, QuitBtnBg, QuitBtnHover, CardSelectedBg);

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
	Header->SetBrushColor(InfoBarBg);
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
		Style.NormalPadding = FMargin(0.f);
		Style.PressedPadding = FMargin(0.f);
		OutButton->SetStyle(Style);
	}

	USizeBox* HitPad = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	HitPad->SetMinDesiredHeight(RecruitIconSize + 8.f);
	OutButton->SetContent(HitPad);
	if (UButtonSlot* BtnSlot = Cast<UButtonSlot>(HitPad->Slot))
	{
		BtnSlot->SetHorizontalAlignment(HAlign_Fill);
		BtnSlot->SetVerticalAlignment(VAlign_Fill);
		BtnSlot->SetPadding(FMargin(0.f));
	}

	UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	Icon->SetBrushSize(FVector2D(RecruitIconSize, RecruitIconSize));
	USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	IconBox->SetWidthOverride(RecruitIconSize);
	IconBox->SetHeightOverride(RecruitIconSize);
	IconBox->SetContent(Icon);
	if (Title == TEXT("Rabbit")) { IconRabbit = Icon; }
	else if (Title == TEXT("Chicken")) { IconChicken = Icon; }
	else if (Title == TEXT("Sheep")) { IconSheep = Icon; }
	else if (Title == TEXT("Pig")) { IconPig = Icon; }

	UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NameText->SetText(FText::FromString(Title));
	NameText->SetColorAndOpacity(FSlateColor(DarkText));
	NameText->SetJustification(ETextJustify::Right);

	OutCostText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	OutCostText->SetText(FText::FromString(TEXT("?")));
	OutCostText->SetColorAndOpacity(FSlateColor(DarkText));
	OutCostText->SetJustification(ETextJustify::Right);

	UVerticalBox* InfoCol = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	{
		UVerticalBoxSlot* NameSlot = InfoCol->AddChildToVerticalBox(NameText);
		NameSlot->SetHorizontalAlignment(HAlign_Right);
		NameSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
	}
	{
		UVerticalBoxSlot* CostSlot = InfoCol->AddChildToVerticalBox(OutCostText);
		CostSlot->SetHorizontalAlignment(HAlign_Right);
	}

	UHorizontalBox* ContentRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	ContentRow->SetVisibility(ESlateVisibility::HitTestInvisible);
	{
		UHorizontalBoxSlot* IconSlot = ContentRow->AddChildToHorizontalBox(IconBox);
		IconSlot->SetPadding(FMargin(4.f, 4.f, 0.f, 4.f));
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}
	{
		USpacer* MidSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
		UHorizontalBoxSlot* SpacerSlot = ContentRow->AddChildToHorizontalBox(MidSpacer);
		SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	{
		UHorizontalBoxSlot* InfoSlot = ContentRow->AddChildToHorizontalBox(InfoCol);
		InfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		InfoSlot->SetPadding(FMargin(0.f, 4.f, 8.f, 4.f));
		InfoSlot->SetVerticalAlignment(VAlign_Center);
		InfoSlot->SetHorizontalAlignment(HAlign_Right);
	}

	UOverlay* SlotOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	{
		UOverlaySlot* BtnOvl = SlotOverlay->AddChildToOverlay(OutButton);
		BtnOvl->SetHorizontalAlignment(HAlign_Fill);
		BtnOvl->SetVerticalAlignment(VAlign_Fill);
	}
	{
		UOverlaySlot* RowOvl = SlotOverlay->AddChildToOverlay(ContentRow);
		RowOvl->SetHorizontalAlignment(HAlign_Fill);
		RowOvl->SetVerticalAlignment(VAlign_Center);
		RowOvl->SetPadding(FMargin(2.f));
	}

	if (UVerticalBox* VParent = Cast<UVerticalBox>(Parent))
	{
		UVerticalBoxSlot* BoxSlot = VParent->AddChildToVerticalBox(SlotOverlay);
		BoxSlot->SetPadding(FMargin(0.f, 3.f));
		BoxSlot->SetHorizontalAlignment(HAlign_Fill);
	}
	else if (UHorizontalBox* HParent = Cast<UHorizontalBox>(Parent))
	{
		UHorizontalBoxSlot* BoxSlot = HParent->AddChildToHorizontalBox(SlotOverlay);
		BoxSlot->SetPadding(FMargin(3.f, 0.f));
		BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	else
	{
		Parent->AddChild(SlotOverlay);
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
	if (BtnRabbit) { BtnRabbit->OnClicked.Clear(); BtnRabbit->OnClicked.AddDynamic(this, &URTSGameHUD::OnRecruitRabbit); }
	if (BtnChicken) { BtnChicken->OnClicked.Clear(); BtnChicken->OnClicked.AddDynamic(this, &URTSGameHUD::OnRecruitChicken); }
	if (BtnSheep) { BtnSheep->OnClicked.Clear(); BtnSheep->OnClicked.AddDynamic(this, &URTSGameHUD::OnRecruitSheep); }
	if (BtnPig) { BtnPig->OnClicked.Clear(); BtnPig->OnClicked.AddDynamic(this, &URTSGameHUD::OnRecruitPig); }
	if (BtnUpgradeRabbit) { BtnUpgradeRabbit->OnClicked.Clear(); BtnUpgradeRabbit->OnClicked.AddDynamic(this, &URTSGameHUD::OnUpgradeRabbit); }
	if (BtnUpgradeChicken) { BtnUpgradeChicken->OnClicked.Clear(); BtnUpgradeChicken->OnClicked.AddDynamic(this, &URTSGameHUD::OnUpgradeChicken); }
	if (BtnUpgradeSheep) { BtnUpgradeSheep->OnClicked.Clear(); BtnUpgradeSheep->OnClicked.AddDynamic(this, &URTSGameHUD::OnUpgradeSheep); }
	if (BtnUpgradePig) { BtnUpgradePig->OnClicked.Clear(); BtnUpgradePig->OnClicked.AddDynamic(this, &URTSGameHUD::OnUpgradePig); }
	if (BtnDialogueAdvance) { BtnDialogueAdvance->OnClicked.Clear(); BtnDialogueAdvance->OnClicked.AddDynamic(this, &URTSGameHUD::OnDialogueClicked); }
	if (BtnPrimaryResult) { BtnPrimaryResult->OnClicked.Clear(); BtnPrimaryResult->OnClicked.AddDynamic(this, &URTSGameHUD::OnPrimaryResult); }
	if (BtnQuit) { BtnQuit->OnClicked.Clear(); BtnQuit->OnClicked.AddDynamic(this, &URTSGameHUD::OnQuit); }
}

void URTSGameHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshTexts();
	RefreshCardHighlights();
	TickUpgradeToast(InDeltaTime);
	UpdateWorldLabels();
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

	RefreshRecruitSlot(BtnRabbit, CostRabbitText, GM->GetEffectiveFodderCost(ERTSUnitType::Rabbit), GM->Fodder);
	RefreshRecruitSlot(BtnChicken, CostChickenText, GM->GetEffectiveFodderCost(ERTSUnitType::Chicken), GM->Fodder);
	RefreshRecruitSlot(BtnSheep, CostSheepText, GM->GetEffectiveFodderCost(ERTSUnitType::Sheep), GM->Fodder);
	RefreshRecruitSlot(BtnPig, CostPigText, GM->GetEffectiveFodderCost(ERTSUnitType::Pig), GM->Fodder);

	RefreshUpgradeSlot(BtnUpgradeRabbit, UpgradeRabbitText, UpgradeCostRabbitText, IconSoulCostRabbit, ERTSUnitType::Rabbit, GM);
	RefreshUpgradeSlot(BtnUpgradeChicken, UpgradeChickenText, UpgradeCostChickenText, IconSoulCostChicken, ERTSUnitType::Chicken, GM);
	RefreshUpgradeSlot(BtnUpgradeSheep, UpgradeSheepText, UpgradeCostSheepText, IconSoulCostSheep, ERTSUnitType::Sheep, GM);
	RefreshUpgradeSlot(BtnUpgradePig, UpgradePigText, UpgradeCostPigText, IconSoulCostPig, ERTSUnitType::Pig, GM);

	ApplyResourceIcons();
}

void URTSGameHUD::RefreshCardHighlights()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	auto TintDispatch = [&](UButton* Btn, UBorder* Card, ERTSUnitType Type)
	{
		const bool bSel = PC && PC->bPlacementPending && PC->PendingRecruitType == Type;
		if (Btn)
		{
			ApplySoftButtonStyle(
				Btn,
				bSel ? CardSelectedBg : DispatchBtnBg,
				bSel ? CardSelectedBg : DispatchBtnHover,
				CardSelectedBg);
		}
		if (Card)
		{
			ApplySoftPanelBrush(Card, bSel ? CardSelectedBg : CardNormalBg);
		}
	};
	TintDispatch(BtnRabbit, UnitCardRabbit, ERTSUnitType::Rabbit);
	TintDispatch(BtnChicken, UnitCardChicken, ERTSUnitType::Chicken);
	TintDispatch(BtnSheep, UnitCardSheep, ERTSUnitType::Sheep);
	TintDispatch(BtnPig, UnitCardPig, ERTSUnitType::Pig);
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
