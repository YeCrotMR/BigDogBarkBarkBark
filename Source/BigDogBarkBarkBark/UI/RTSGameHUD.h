// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RTSUnitTypes.h"
#include "RTSGameHUD.generated.h"

class UTextBlock;
class UButton;
class UVerticalBox;
class UHorizontalBox;
class ARTSGameMode;
class ARTSPlayerController;

UCLASS()
class BIGDOGBARKBARKBARK_API URTSGameHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	void RebuildLayout();
	void RefreshTexts();

	UFUNCTION()
	void OnRecruitRabbit();
	UFUNCTION()
	void OnRecruitChicken();
	UFUNCTION()
	void OnRecruitSheep();
	UFUNCTION()
	void OnRecruitPig();
	UFUNCTION()
	void OnLane0();
	UFUNCTION()
	void OnLane1();
	UFUNCTION()
	void OnModeCombat();
	UFUNCTION()
	void OnModeCollect();

	UButton* MakeButton(UPanelWidget* Parent, const FString& Label, FName Name);

	UPROPERTY()
	UTextBlock* FodderText = nullptr;

	UPROPERTY()
	UTextBlock* StatusText = nullptr;

	UPROPERTY()
	UTextBlock* SelectionText = nullptr;

	UPROPERTY()
	UVerticalBox* RootBox = nullptr;
};
