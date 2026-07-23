// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSCameraPawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"

ARTSCameraPawn::ARTSCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Focus pawn is a free-look point — never collide with void fillers / props.
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SetActorEnableCollision(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(SceneRoot);
	SpringArm->TargetArmLength = 2400.f;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bInheritRoll = false;
	SpringArm->SetRelativeRotation(FRotator(-70.f, -90.f, 0.f));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->MaxSpeed = MoveSpeed;
	// Don't sweep against WorldStatic (outside meshes would shrink the pan range).
	Movement->bSnapToPlaneAtStart = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

void ARTSCameraPawn::BeginPlay()
{
	Super::BeginPlay();
	SetActorEnableCollision(false);
	SpawnLocation = GetActorLocation();
	ClampLateralPosition();
}

void ARTSCameraPawn::SetFocusLocation(const FVector& WorldLocation)
{
	SetActorLocation(WorldLocation, false, nullptr, ETeleportType::TeleportPhysics);
	SpawnLocation = WorldLocation;
	ClampLateralPosition();
}

void ARTSCameraPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ClampLateralPosition();
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

	// Direct move — bypasses FloatingPawnMovement sweeps that hit outside geometry.
	const FVector MoveDir = Camera
		? FVector::VectorPlaneProject(Camera->GetRightVector(), FVector::UpVector).GetSafeNormal()
		: GetActorRightVector();
	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;
	const FVector Delta = MoveDir * (Value * MoveSpeed * DeltaSeconds);
	SetActorLocation(GetActorLocation() + Delta, false, nullptr, ETeleportType::TeleportPhysics);
	ClampLateralPosition();
}

void ARTSCameraPawn::ZoomCamera(float Value)
{
	if (!SpringArm || FMath::IsNearlyZero(Value))
	{
		return;
	}
	SpringArm->TargetArmLength = FMath::Clamp(SpringArm->TargetArmLength - Value * ZoomSpeed, MinArmLength, MaxArmLength);
}

void ARTSCameraPawn::ClampLateralPosition()
{
	if (!bClampLateralPan)
	{
		return;
	}

	const float MinX = FMath::Min(LateralPanMin, LateralPanMax);
	const float MaxX = FMath::Max(LateralPanMin, LateralPanMax);

	FVector Loc = GetActorLocation();
	const float ClampedX = FMath::Clamp(Loc.X, MinX, MaxX);
	if (!FMath::IsNearlyEqual(Loc.X, ClampedX))
	{
		Loc.X = ClampedX;
		SetActorLocation(Loc, false, nullptr, ETeleportType::TeleportPhysics);
	}
}
