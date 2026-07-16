// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTSLaneSpline.generated.h"

class USplineComponent;
class ARTSResourceNode;

UCLASS()
class BIGDOGBARKBARKBARK_API ARTSLaneSpline : public AActor
{
	GENERATED_BODY()

public:
	ARTSLaneSpline();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Lane")
	USplineComponent* Spline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Lane")
	int32 LaneIndex = 0;

	UFUNCTION(BlueprintCallable, Category = "RTS|Lane")
	float GetSplineLength() const;

	UFUNCTION(BlueprintCallable, Category = "RTS|Lane")
	FVector GetLocationAtDistance(float Distance) const;

	UFUNCTION(BlueprintCallable, Category = "RTS|Lane")
	FRotator GetRotationAtDistance(float Distance) const;

	/** Closest spline distance to a world position (for coop / node placement). */
	UFUNCTION(BlueprintCallable, Category = "RTS|Lane")
	float GetDistanceForWorldLocation(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintCallable, Category = "RTS|Lane")
	ARTSResourceNode* FindNearestResourceNode(float FromDistance) const;

	void RegisterResourceNode(ARTSResourceNode* Node);
	void UnregisterResourceNode(ARTSResourceNode* Node);

	const TArray<TWeakObjectPtr<ARTSResourceNode>>& GetResourceNodes() const { return ResourceNodes; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TArray<TWeakObjectPtr<ARTSResourceNode>> ResourceNodes;
};
