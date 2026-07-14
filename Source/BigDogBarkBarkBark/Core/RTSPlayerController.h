// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RTSUnitTypes.h"
#include "RTSPlayerController.generated.h"

class ARTSUnitBase;
class URTSGameHUD;
class ARTSGameMode;

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

	UFUNCTION(BlueprintCallable, Category = "RTS|Recruit")
	void SetSelectedLane(int32 LaneIndex);

	UFUNCTION(BlueprintCallable, Category = "RTS|Recruit")
	void RecruitSelectedType();

	UFUNCTION(BlueprintCallable, Category = "RTS|Recruit")
	void RecruitType(ERTSUnitType Type);

	UFUNCTION(BlueprintCallable, Category = "RTS|Unit")
	void SetSelectedUnitWorkMode(EUnitWorkMode Mode);

	UFUNCTION(BlueprintCallable, Category = "RTS|Unit")
	void ToggleSelectedUnitWorkMode();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	void OnLeftClick();
	void SelectUnitUnderCursor();
	void CreateHUD();
	ARTSUnitBase* FindNearestFarmUnit() const;

	void HotkeyRecruitRabbit();
	void HotkeyRecruitChicken();
	void HotkeyRecruitSheep();
	void HotkeyRecruitPig();
	void HotkeyLane0();
	void HotkeyLane1();
	void HotkeyToggleMode();
};
