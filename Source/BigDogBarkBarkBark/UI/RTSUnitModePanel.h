// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RTSUnitTypes.h"
#include "RTSUnitModePanel.generated.h"

class UButton;
class UTextBlock;
class UHorizontalBox;
class UBorder;
class UFont;
class UTexture2D;
class ARTSUnitModeRing;

UCLASS()
class BIGDOGBARKBARKBARK_API URTSUnitModePanel : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetOwnerRing(ARTSUnitModeRing* InRing);
	void RefreshModeHighlight(EUnitWorkMode Mode);

protected:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	void BuildLayout();
	void EnsureStyleAssets();
	UTexture2D* CreateRoundPanelTexture();
	void ApplyRoundPanelBrush(UBorder* Border, const FLinearColor& Tint);
	void ApplyRoundButtonStyle(UButton* Btn, const FLinearColor& Normal, const FLinearColor& Hovered);
	void ApplyModeFont(UTextBlock* Text, int32 Size) const;

	UFUNCTION()
	void OnCombatClicked();

	UFUNCTION()
	void OnCollectClicked();

	UPROPERTY()
	UButton* BtnCombat = nullptr;

	UPROPERTY()
	UButton* BtnCollect = nullptr;

	UPROPERTY()
	TWeakObjectPtr<ARTSUnitModeRing> OwnerRing;

	UPROPERTY()
	UFont* ModeFont = nullptr;

	UPROPERTY()
	UTexture2D* RoundPanelTexture = nullptr;

	bool bBuilt = false;
};
