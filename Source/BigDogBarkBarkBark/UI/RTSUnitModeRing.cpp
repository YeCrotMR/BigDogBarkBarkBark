// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSUnitModeRing.h"
#include "RTSUnitModePanel.h"
#include "RTSUnitBase.h"
#include "RTSPlayerController.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"

ARTSUnitModeRing::ARTSUnitModeRing()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	RingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RingMesh"));
	RingMesh->SetupAttachment(SceneRoot);
	RingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RingMesh->SetCastShadow(false);
	RingMesh->SetRelativeScale3D(FVector(2.2f, 2.2f, 0.04f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	if (CylinderMesh.Succeeded())
	{
		RingMesh->SetStaticMesh(CylinderMesh.Object);
	}
	if (ShapeMat.Succeeded())
	{
		RingMesh->SetMaterial(0, ShapeMat.Object);
	}

	ModeWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("ModeWidget"));
	ModeWidget->SetupAttachment(SceneRoot);
	// Screen space: always faces camera and keeps a reliable clickable size.
	ModeWidget->SetWidgetSpace(EWidgetSpace::Screen);
	ModeWidget->SetDrawAtDesiredSize(false);
	ModeWidget->SetDrawSize(FVector2D(260.f, 64.f));
	// Anchor at top-center so the panel hangs below the offset point.
	ModeWidget->SetPivot(FVector2D(0.5f, 0.f));
	ModeWidget->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ModeWidget->SetRelativeLocation(FVector(0.f, -140.f, 12.f));
	ModeWidget->SetRelativeRotation(FRotator::ZeroRotator);
	ModeWidget->SetWidgetClass(URTSUnitModePanel::StaticClass());
}

void ARTSUnitModeRing::BeginPlay()
{
	Super::BeginPlay();

	if (RingMesh)
	{
		if (UMaterialInterface* Parent = RingMesh->GetMaterial(0))
		{
			if (UMaterialInstanceDynamic* MID = RingMesh->CreateDynamicMaterialInstance(0, Parent))
			{
				const FLinearColor Tint(0.25f, 0.85f, 1.f, 0.55f);
				MID->SetVectorParameterValue(TEXT("Color"), Tint);
				MID->SetVectorParameterValue(TEXT("BaseColor"), Tint);
			}
		}
	}

	if (ModeWidget)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			ModeWidget->SetOwnerPlayer(PC->GetLocalPlayer());
		}
		ModeWidget->InitWidget();
		ModePanel = Cast<URTSUnitModePanel>(ModeWidget->GetUserWidgetObject());
		if (ModePanel)
		{
			ModePanel->SetOwnerRing(this);
			ModePanel->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

void ARTSUnitModeRing::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ARTSUnitBase* Unit = FollowUnit.Get();
	if (!Unit || !Unit->IsAlive())
	{
		Destroy();
		return;
	}
	SyncTransformToUnit();
	RefreshPanelHighlight();
}

void ARTSUnitModeRing::SetFollowUnit(ARTSUnitBase* Unit)
{
	FollowUnit = Unit;
	SyncTransformToUnit();
	RefreshPanelHighlight();
}

void ARTSUnitModeRing::SyncTransformToUnit()
{
	ARTSUnitBase* Unit = FollowUnit.Get();
	if (!Unit)
	{
		return;
	}
	const FVector Loc = Unit->GetActorLocation() + FVector(0.f, 0.f, GroundOffsetZ - 60.f);
	SetActorLocation(Loc);
	// Keep ring axis-aligned on ground (no pitch/roll).
	SetActorRotation(FRotator(0.f, 0.f, 0.f));

	if (!ModeWidget)
	{
		return;
	}

	// Push buttons toward the bottom of the screen so they don't cover the unit mesh.
	FVector FlatDown(0.f, -1.f, 0.f);
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);
		const FVector CamUp = CamRot.RotateVector(FVector::UpVector);
		FlatDown = FVector(-CamUp.X, -CamUp.Y, 0.f);
		if (!FlatDown.Normalize())
		{
			const FVector CamRight = CamRot.RotateVector(FVector::RightVector);
			FlatDown = FVector(-CamRight.Y, CamRight.X, 0.f);
			if (!FlatDown.Normalize())
			{
				FlatDown = FVector(0.f, -1.f, 0.f);
			}
		}
	}

	ModeWidget->SetWorldLocation(Unit->GetActorLocation() + FlatDown * ButtonScreenOffset + FVector(0.f, 0.f, 10.f));
}

void ARTSUnitModeRing::RefreshPanelHighlight()
{
	ARTSUnitBase* Unit = FollowUnit.Get();
	if (ModePanel && Unit)
	{
		ModePanel->RefreshModeHighlight(Unit->WorkMode);
	}
}

void ARTSUnitModeRing::RefreshModeHighlight()
{
	RefreshPanelHighlight();
}

void ARTSUnitModeRing::ApplyWorkMode(EUnitWorkMode Mode)
{
	ARTSUnitBase* Unit = FollowUnit.Get();
	if (!Unit || !Unit->IsAlive())
	{
		return;
	}

	if (ARTSPlayerController* PC = Cast<ARTSPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		if (PC->SelectedUnit == Unit)
		{
			PC->SetSelectedUnitWorkMode(Mode);
		}
		else
		{
			Unit->SetWorkMode(Mode);
		}
	}
	else
	{
		Unit->SetWorkMode(Mode);
	}
	RefreshPanelHighlight();
}
