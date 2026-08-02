// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTSLaneSpline.generated.h"

class USplineComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class ARTSResourceNode;

UCLASS()
class BIGDOGBARKBARKBARK_API ARTSLaneSpline : public AActor
{
	GENERATED_BODY()

public:
	ARTSLaneSpline();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Lane")
	USplineComponent* Spline;

	/** Runtime-visible lane highlight (works in packaged builds; DrawDebug does not). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Lane|Highlight")
	UInstancedStaticMeshComponent* HighlightSegments;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Lane|Highlight")
	UInstancedStaticMeshComponent* HighlightEnds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Lane")
	int32 LaneIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Lane|Highlight")
	FLinearColor HighlightColor = FLinearColor(0.2f, 0.95f, 1.f, 1.f);

	/** World-unit width of the highlight ribbon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Lane|Highlight")
	float HighlightThickness = 40.f;

	/** Lift highlight above the ground so it stays readable top-down. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Lane|Highlight")
	float HighlightZOffset = 40.f;

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

	UFUNCTION(BlueprintCallable, Category = "RTS|Lane")
	void SetHighlighted(bool bInHighlighted);

	UFUNCTION(BlueprintCallable, Category = "RTS|Lane")
	bool IsHighlighted() const { return bHighlighted; }

	void RegisterResourceNode(ARTSResourceNode* Node);
	void UnregisterResourceNode(ARTSResourceNode* Node);

	const TArray<TWeakObjectPtr<ARTSResourceNode>>& GetResourceNodes() const { return ResourceNodes; }

protected:
	virtual void BeginPlay() override;

	void EnsureHighlightAssets();
	void RebuildHighlightMeshes();
	void ClearHighlightMeshes();

	UPROPERTY()
	TArray<TWeakObjectPtr<ARTSResourceNode>> ResourceNodes;

	UPROPERTY()
	UMaterialInstanceDynamic* HighlightMID = nullptr;

	bool bHighlighted = false;
	bool bHighlightAssetsReady = false;
};
