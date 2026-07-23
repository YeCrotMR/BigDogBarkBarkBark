// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RTSCameraPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UFloatingPawnMovement;

UCLASS()
class BIGDOGBARKBARKBARK_API ARTSCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ARTSCameraPawn();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Camera")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Camera")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Camera")
	UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Camera")
	UFloatingPawnMovement* Movement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Camera")
	float MoveSpeed = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Camera")
	float ZoomSpeed = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Camera")
	float MinArmLength = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Camera")
	float MaxArmLength = 3500.f;

	/** Clamp A/D pan relative to spawn along world X (screen left/right with default yaw). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Camera|Bounds")
	bool bClampLateralPan = true;

	/** How far left of spawn the camera may go (negative = left). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Camera|Bounds")
	float LateralPanMin = -2000.f;

	/** How far right of spawn the camera may go (positive = right). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Camera|Bounds")
	float LateralPanMax = 2000.f;

	void MoveRight(float Value);
	void ZoomCamera(float Value);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void ClampLateralPosition();

	FVector SpawnLocation = FVector::ZeroVector;
};
