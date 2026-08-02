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
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "TextureResource.h"

namespace
{
	FLinearColor ModeFrameBg(0.12f, 0.14f, 0.18f, 0.94f);
	FLinearColor ModeActive(0.15f, 0.55f, 1.f, 0.96f);
	FLinearColor ModeIdle(0.22f, 0.24f, 0.28f, 0.92f);
	FLinearColor ModeIdleHover(0.35f, 0.38f, 0.42f, 0.95f);
	FLinearColor ModeText(0.95f, 0.95f, 0.97f, 1.f);
}

void URTSUnitModePanel::SetOwnerRing(ARTSUnitModeRing* InRing)
{
	OwnerRing = InRing;
}

void URTSUnitModePanel::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureStyleAssets();
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

void URTSUnitModePanel::EnsureStyleAssets()
{
	if (!ModeFont)
	{
		ModeFont = LoadObject<UFont>(nullptr, TEXT("/Game/Fonts/Galdeano.Galdeano"));
		if (!ModeFont)
		{
			ModeFont = LoadObject<UFont>(nullptr, TEXT("/Game/Fonts/Galdeanofont.Galdeanofont"));
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

UTexture2D* URTSUnitModePanel::CreateRoundPanelTexture()
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

void URTSUnitModePanel::ApplyRoundPanelBrush(UBorder* Border, const FLinearColor& Tint)
{
	if (!Border)
	{
		return;
	}
	EnsureStyleAssets();

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
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.TintColor = FSlateColor(Tint);
	}
	Border->SetBrush(Brush);
}

void URTSUnitModePanel::ApplyRoundButtonStyle(UButton* Btn, const FLinearColor& Normal, const FLinearColor& Hovered)
{
	if (!Btn)
	{
		return;
	}
	EnsureStyleAssets();

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
		Style.Pressed = MakeBrush(Hovered);
		Style.Disabled = MakeBrush(FLinearColor(Normal.R, Normal.G, Normal.B, 0.4f));
	}
	else
	{
		Style.Normal.TintColor = FSlateColor(Normal);
		Style.Hovered.TintColor = FSlateColor(Hovered);
		Style.Pressed.TintColor = FSlateColor(Hovered);
	}
	Style.NormalPadding = FMargin(8.f, 8.f);
	Style.PressedPadding = FMargin(8.f, 8.f);
	Btn->SetStyle(Style);
}

void URTSUnitModePanel::ApplyModeFont(UTextBlock* Text, int32 Size) const
{
	if (!Text || !ModeFont)
	{
		return;
	}
	FSlateFontInfo Info = Text->Font;
	Info.FontObject = ModeFont;
	Info.Size = Size;
	Text->SetFont(Info);
}

void URTSUnitModePanel::BuildLayout()
{
	if (bBuilt || !WidgetTree)
	{
		return;
	}
	bBuilt = true;
	EnsureStyleAssets();

	USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSize"));
	RootSize->SetWidthOverride(260.f);
	RootSize->SetHeightOverride(58.f);
	WidgetTree->RootWidget = RootSize;

	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Frame"));
	ApplyRoundPanelBrush(Frame, ModeFrameBg);
	Frame->SetPadding(FMargin(6.f, 5.f));
	RootSize->AddChild(Frame);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ModeRow"));
	Frame->SetContent(Row);

	auto MakeBtn = [&](const FString& Label, FName Name) -> UButton*
	{
		UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(FText::FromString(Label));
		Text->SetJustification(ETextJustify::Center);
		Text->SetColorAndOpacity(FSlateColor(ModeText));
		ApplyModeFont(Text, 16);
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
	EnsureStyleAssets();
	ApplyRoundButtonStyle(BtnCombat, Mode == EUnitWorkMode::Combat ? ModeActive : ModeIdle,
		Mode == EUnitWorkMode::Combat ? ModeActive : ModeIdleHover);
	ApplyRoundButtonStyle(BtnCollect, Mode == EUnitWorkMode::Collect ? ModeActive : ModeIdle,
		Mode == EUnitWorkMode::Collect ? ModeActive : ModeIdleHover);
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
