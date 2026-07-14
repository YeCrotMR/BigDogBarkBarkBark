// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSBaseBuilding.h"
#include "RTSGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

ARTSBaseBuilding::ARTSBaseBuilding()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
		Mesh->SetWorldScale3D(FVector(2.f, 2.f, 2.f));
	}
}

void ARTSBaseBuilding::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
}

void ARTSBaseBuilding::ReceiveDamage(float Amount, AActor* DamageCauser)
{
	if (!IsAlive())
	{
		return;
	}

	CurrentHealth = FMath::Max(0.f, CurrentHealth - Amount);
	if (CurrentHealth <= 0.f && bIsCoreBuilding)
	{
		if (ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GM->NotifyCoreDestroyed();
		}
	}
}
