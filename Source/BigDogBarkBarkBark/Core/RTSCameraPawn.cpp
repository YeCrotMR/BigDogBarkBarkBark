// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSCameraPawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"

ARTSCameraPawn::ARTSCameraPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(SceneRoot);
	SpringArm->TargetArmLength = 2400.f;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bInheritRoll = false;
	// Pitch: top-down tilt. Yaw +90 = view rotated 90° CCW when looking from above.
	SpringArm->SetRelativeRotation(FRotator(-70.f, -90.f, 0.f));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->MaxSpeed = MoveSpeed;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

void ARTSCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis("CameraMoveRight", this, &ARTSCameraPawn::MoveRight);
	PlayerInputComponent->BindAxis("CameraZoom", this, &ARTSCameraPawn::ZoomCamera);
}

void ARTSCameraPawn::MoveRight(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}
	// Flatten camera right onto ground so A/D stay screen-left/right after yaw turn.
	const FVector MoveDir = Camera
		? FVector::VectorPlaneProject(Camera->GetRightVector(), FVector::UpVector).GetSafeNormal()
		: GetActorRightVector();
	AddMovementInput(MoveDir, Value);
}

void ARTSCameraPawn::ZoomCamera(float Value)
{
	if (!SpringArm || FMath::IsNearlyZero(Value))
	{
		return;
	}
	SpringArm->TargetArmLength = FMath::Clamp(SpringArm->TargetArmLength - Value * ZoomSpeed, MinArmLength, MaxArmLength);
}
