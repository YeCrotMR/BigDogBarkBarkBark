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
