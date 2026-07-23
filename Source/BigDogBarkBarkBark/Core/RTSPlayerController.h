// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RTSUnitTypes.h"
#include "RTSPlayerController.generated.h"

class ARTSUnitBase;
class URTSGameHUD;
class ARTSGameMode;
class ARTSLaneSpline;
class ARTSUnitModeRing;

UCLASS()
class BIGDOGBARKBARKBARK_API ARTSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARTSPlayerController();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|UI")
	TSubclassOf<URTSGameHUD> HUDWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|UI")
	URTSGameHUD* HUDWidget = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Selection")
	ARTSUnitBase* SelectedUnit = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Recruit")
	int32 SelectedLaneIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Recruit")
	ERTSUnitType PendingRecruitType = ERTSUnitType::Chicken;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Recruit")
	bool bPlacementPending = false;

	UFUNCTION(BlueprintCallable, Category = "RTS|Recruit")
	void SetSelectedLane(int32 LaneIndex);

	UFUNCTION(BlueprintCallable, Category = "RTS|Recruit")
	void RecruitSelectedType();

	/** Immediate recruit on current SelectedLaneIndex (hotkeys / debug). */
	UFUNCTION(BlueprintCallable, Category = "RTS|Recruit")
	void RecruitType(ERTSUnitType Type);

	/** PvZ card pick: enter placement mode (does not spawn yet). */
	UFUNCTION(BlueprintCallable, Category = "RTS|Recruit")
	void SelectRecruitCard(ERTSUnitType Type);

	UFUNCTION(BlueprintCallable, Category = "RTS|Recruit")
	void ConfirmRecruitOnLane(int32 LaneIndex);

	UFUNCTION(BlueprintCallable, Category = "RTS|Recruit")
	void CancelPlacement();

	UFUNCTION(BlueprintCallable, Category = "RTS|Unit")
	void SetSelectedUnitWorkMode(EUnitWorkMode Mode);

	UFUNCTION(BlueprintCallable, Category = "RTS|Unit")
	void ToggleSelectedUnitWorkMode();

	UFUNCTION(BlueprintCallable, Category = "RTS|Progression")
	void UpgradeUnitType(ERTSUnitType Type);

	UFUNCTION(BlueprintCallable, Category = "RTS|Lane")
	int32 FindNearestLaneIndex(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintCallable, Category = "RTS|Lane")
	void SetHighlightedLane(int32 LaneIndex);

	UFUNCTION(BlueprintCallable, Category = "RTS|Lane")
	void ClearLaneHighlights();

	/** Currently highlighted lane while placing (-1 = none). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Recruit")
	int32 HighlightedLaneIndex = -1;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;

	void OnLeftClick();
	void OnRightClick();
	void OnCancelPlacementKey();
	void OnAdvanceDialogueKey();
	void SelectUnitUnderCursor();
	void HandleWorldClickForPlacement();
	void CreateHUD();
	void ClearSelectedUnit();
	void ShowModeRingForSelected();
	void DestroyModeRing();
	void UpdatePlacementLaneHighlight();
	bool GetMouseWorldProbe(FVector& OutProbe) const;
	bool IsPointerOverInteractiveUI() const;
	ARTSUnitBase* FindNearestFarmUnit() const;
	ARTSUnitBase* SpawnRecruit(ERTSUnitType Type, int32 LaneIndex);

	UPROPERTY()
	TWeakObjectPtr<ARTSUnitModeRing> ModeRing;

	void HotkeyRecruitRabbit();
	void HotkeyRecruitChicken();
	void HotkeyRecruitSheep();
	void HotkeyRecruitPig();
	void HotkeyLane0();
	void HotkeyLane1();
	void HotkeyToggleMode();
	void HotkeyUpgradeRabbit();
	void HotkeyUpgradeChicken();
	void HotkeyUpgradeSheep();
	void HotkeyUpgradePig();
};
