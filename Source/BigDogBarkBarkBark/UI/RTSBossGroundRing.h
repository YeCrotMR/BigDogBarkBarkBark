// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTSBossGroundRing.generated.h"

class ARTSUnitBase;
class USceneComponent;
class UStaticMeshComponent;

/** Persistent red ground ring under boss animals. */
UCLASS()
class BIGDOGBARKBARKBARK_API ARTSBossGroundRing : public AActor
{
	GENERATED_BODY()

public:
	ARTSBossGroundRing();

	UFUNCTION(BlueprintCallable, Category = "RTS|UI")
	void SetFollowUnit(ARTSUnitBase* Unit);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void SyncTransformToUnit();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|UI")
	USceneComponent* SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|UI")
	UStaticMeshComponent* RingMesh = nullptr;

	UPROPERTY()
	TWeakObjectPtr<ARTSUnitBase> FollowUnit;

	UPROPERTY(EditAnywhere, Category = "RTS|UI")
	float GroundOffsetZ = 8.f;
};
