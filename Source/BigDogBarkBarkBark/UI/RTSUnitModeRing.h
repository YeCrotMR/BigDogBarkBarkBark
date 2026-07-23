// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTSUnitTypes.h"
#include "RTSUnitModeRing.generated.h"

class ARTSUnitBase;
class USceneComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class URTSUnitModePanel;

UCLASS()
class BIGDOGBARKBARKBARK_API ARTSUnitModeRing : public AActor
{
	GENERATED_BODY()

public:
	ARTSUnitModeRing();

	UFUNCTION(BlueprintCallable, Category = "RTS|UI")
	void SetFollowUnit(ARTSUnitBase* Unit);

	UFUNCTION(BlueprintCallable, Category = "RTS|UI")
	ARTSUnitBase* GetFollowUnit() const { return FollowUnit.Get(); }

	UFUNCTION(BlueprintCallable, Category = "RTS|UI")
	void ApplyWorkMode(EUnitWorkMode Mode);

	UFUNCTION(BlueprintCallable, Category = "RTS|UI")
	void RefreshModeHighlight();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void SyncTransformToUnit();
	void RefreshPanelHighlight();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|UI")
	USceneComponent* SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|UI")
	UStaticMeshComponent* RingMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|UI")
	UWidgetComponent* ModeWidget = nullptr;

	UPROPERTY()
	URTSUnitModePanel* ModePanel = nullptr;

	UPROPERTY()
	TWeakObjectPtr<ARTSUnitBase> FollowUnit;

	UPROPERTY(EditAnywhere, Category = "RTS|UI")
	float GroundOffsetZ = 8.f;

	UPROPERTY(EditAnywhere, Category = "RTS|UI")
	float ButtonOffsetY = -90.f;
};
