// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTSResourceNode.generated.h"

class ARTSLaneSpline;
class UStaticMeshComponent;

UCLASS()
class BIGDOGBARKBARKBARK_API ARTSResourceNode : public AActor
{
	GENERATED_BODY()

public:
	ARTSResourceNode();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Resource")
	UStaticMeshComponent* MarkerMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Resource")
	ARTSLaneSpline* OwnerLane = nullptr;

	/** Distance along the owner lane spline where this node sits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Resource", meta = (ClampMin = "0.0"))
	float SplineDistance = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Resource")
	float ArrivalTolerance = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Resource")
	float BaseFodderPerTick = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Resource")
	float CollectInterval = 3.f;

	UFUNCTION(BlueprintCallable, Category = "RTS|Resource")
	void SnapToLane();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
