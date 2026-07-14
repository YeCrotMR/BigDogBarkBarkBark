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

	void MoveRight(float Value);
	void ZoomCamera(float Value);

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
};
