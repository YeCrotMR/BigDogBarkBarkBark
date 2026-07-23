// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RTSUnitTypes.h"
#include "RTSUnitModePanel.generated.h"

class UButton;
class UTextBlock;
class UHorizontalBox;
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

	bool bBuilt = false;
};
