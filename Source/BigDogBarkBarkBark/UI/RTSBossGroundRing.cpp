// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSBossGroundRing.h"
#include "RTSUnitBase.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"

ARTSBossGroundRing::ARTSBossGroundRing()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	RingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RingMesh"));
	RingMesh->SetupAttachment(SceneRoot);
	RingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RingMesh->SetCastShadow(false);
	RingMesh->SetRelativeScale3D(FVector(3.4f, 3.4f, 0.05f));

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
}

void ARTSBossGroundRing::SetFollowUnit(ARTSUnitBase* Unit)
{
	FollowUnit = Unit;
	SyncTransformToUnit();
}

void ARTSBossGroundRing::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ARTSUnitBase* Unit = FollowUnit.Get();
	if (!Unit || !Unit->IsAlive())
	{
		Destroy();
		return;
	}
	SyncTransformToUnit();
}

void ARTSBossGroundRing::SyncTransformToUnit()
{
	ARTSUnitBase* Unit = FollowUnit.Get();
	if (!Unit)
	{
		return;
	}

	SetActorLocation(Unit->GetActorLocation() + FVector(0.f, 0.f, GroundOffsetZ - 60.f));
	SetActorRotation(FRotator::ZeroRotator);
}

void ARTSBossGroundRing::BeginPlay()
{
	Super::BeginPlay();

	if (RingMesh)
	{
		if (UMaterialInterface* Parent = RingMesh->GetMaterial(0))
		{
			if (UMaterialInstanceDynamic* MID = RingMesh->CreateDynamicMaterialInstance(0, Parent))
			{
				const FLinearColor Tint(0.95f, 0.12f, 0.1f, 0.7f);
				MID->SetVectorParameterValue(TEXT("Color"), Tint);
				MID->SetVectorParameterValue(TEXT("BaseColor"), Tint);
			}
		}
	}
}
