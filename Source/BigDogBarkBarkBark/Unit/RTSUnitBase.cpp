// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSUnitBase.h"
#include "RTSLaneSpline.h"
#include "RTSResourceNode.h"
#include "RTSBaseBuilding.h"
#include "RTSGameMode.h"
#include "RTSBossGroundRing.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

ARTSUnitBase::ARTSUnitBase()
{
	PrimaryActorTick.bCanEverTick = true;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	SetRootComponent(Capsule);
	Capsule->InitCapsuleSize(55.f, 70.f);
	Capsule->SetCollisionProfileName(TEXT("Pawn"));
	Capsule->SetCanEverAffectNavigation(false);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Capsule);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
	PlaceholderMesh->SetupAttachment(Capsule);
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PlaceholderMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	PlaceholderMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	PlaceholderMesh->SetRelativeLocation(FVector(0.f, 0.f, -20.f));
	PlaceholderMesh->SetRelativeScale3D(FVector(1.1f, 1.1f, 1.4f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	if (SphereMesh.Succeeded())
	{
		FarmPlaceholderMesh = SphereMesh.Object;
		PlaceholderMesh->SetStaticMesh(FarmPlaceholderMesh);
	}
	if (CubeMesh.Succeeded())
	{
		WildPlaceholderMesh = CubeMesh.Object;
	}
	if (ShapeMat.Succeeded())
	{
		PlaceholderMaterial = ShapeMat.Object;
		PlaceholderMesh->SetMaterial(0, PlaceholderMaterial);
	}

	AutoPossessAI = EAutoPossessAI::Disabled;
}

void ARTSUnitBase::BeginPlay()
{
	Super::BeginPlay();
	if (Stats.UnitType == ERTSUnitType::None && UnitType != ERTSUnitType::None)
	{
		InitializeUnit(UnitType, AssignedLane, DistanceAlongSpline, WorkMode);
	}
}

void ARTSUnitBase::InitializeUnit(ERTSUnitType InType, ARTSLaneSpline* Lane, float StartDistance, EUnitWorkMode InMode)
{
	UnitType = InType;
	Stats = RTSUnitData::GetDefaultStats(InType);
	Team = RTSUnitData::GetTeam(InType);
	CurrentHealth = Stats.MaxHealth;
	AssignedLane = Lane;
	DistanceAlongSpline = StartDistance;
	WorkMode = InMode;
	bReachedLaneEnd = false;
	bHasUsedAmbush = false;
	bFoxBossSummonTriggered = false;
	EffectiveAttackCooldown = AttackCooldown;
	SplineTravelDir = (Team == ERTSTeam::Farm) ? 1.f : -1.f;
	ApplyWorkModeCollision();
	ApplyTeamAppearance();
	CacheWalkAnimFromMesh();
	PlayWalkAnimation();

	if (AssignedLane)
	{
		ApplySplineTransform();
	}

	if (Team == ERTSTeam::Farm && WorkMode == EUnitWorkMode::Collect)
	{
		TargetResource = AssignedLane ? AssignedLane->FindNearestResourceNode(DistanceAlongSpline) : nullptr;
		bMovingTowardResource = true;
		EnterState(ERTSUnitState::Moving);
	}
	else
	{
		EnterState(ERTSUnitState::Moving);
	}

	if (Stats.bIsBoss)
	{
		EnsureBossRing();
	}
	else
	{
		DestroyBossRing();
	}
}

void ARTSUnitBase::EnsureBossRing()
{
	if (BossRing.IsValid())
	{
		BossRing->SetFollowUnit(this);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;
	ARTSBossGroundRing* Ring = World->SpawnActor<ARTSBossGroundRing>(
		ARTSBossGroundRing::StaticClass(),
		GetActorLocation(),
		FRotator::ZeroRotator,
		Params);
	if (Ring)
	{
		Ring->SetFollowUnit(this);
		BossRing = Ring;
	}
}

void ARTSUnitBase::DestroyBossRing()
{
	if (ARTSBossGroundRing* Ring = BossRing.Get())
	{
		Ring->Destroy();
	}
	BossRing.Reset();
}

void ARTSUnitBase::SetWorkMode(EUnitWorkMode NewMode)
{
	if (Team != ERTSTeam::Farm || !IsAlive() || WorkMode == NewMode)
	{
		return;
	}

	WorkMode = NewMode;
	CurrentTarget = nullptr;
	ApplyWorkModeCollision();
	ApplyTeamAppearance();

	if (WorkMode == EUnitWorkMode::Collect)
	{
		bReachedLaneEnd = false;
		TargetResource = AssignedLane ? AssignedLane->FindNearestResourceNode(DistanceAlongSpline) : nullptr;
		bMovingTowardResource = true;
		CollectTimer = 0.f;
		EnterState(ERTSUnitState::Moving);
	}
	else
	{
		TargetResource = nullptr;
		EnterState(bReachedLaneEnd ? ERTSUnitState::Guarding : ERTSUnitState::Moving);
	}
}

void ARTSUnitBase::ApplyWorkModeCollision()
{
	if (!Capsule)
	{
		return;
	}

	// Spline-driven movement: no physics block, but keep Visibility block for mouse select.
	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Capsule->SetCollisionObjectType(ECC_WorldDynamic);
	Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
	Capsule->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

	if (PlaceholderMesh)
	{
		PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		PlaceholderMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		PlaceholderMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		PlaceholderMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
	}
}

void ARTSUnitBase::ApplyTeamAppearance()
{
	// Blueprint assigned a real skeletal mesh → hide placeholder ball/cube.
	const bool bHasAnimalMesh = Mesh && Mesh->SkeletalMesh != nullptr;
	if (bHasAnimalMesh)
	{
		if (PlaceholderMesh)
		{
			PlaceholderMesh->SetVisibility(false);
			PlaceholderMesh->SetHiddenInGame(true);
		}
		if (Mesh)
		{
			Mesh->SetVisibility(true);
			Mesh->SetHiddenInGame(false);
		}
		return;
	}

	if (!PlaceholderMesh)
	{
		return;
	}

	PlaceholderMesh->SetVisibility(true);
	PlaceholderMesh->SetHiddenInGame(false);

	if (Team == ERTSTeam::Wild && WildPlaceholderMesh)
	{
		PlaceholderMesh->SetStaticMesh(WildPlaceholderMesh);
		PlaceholderMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.2f));
	}
	else if (FarmPlaceholderMesh)
	{
		PlaceholderMesh->SetStaticMesh(FarmPlaceholderMesh);
		PlaceholderMesh->SetRelativeScale3D(FVector(1.1f, 1.1f, 1.4f));
	}

	UMaterialInterface* ParentMat = PlaceholderMaterial
		? PlaceholderMaterial
		: PlaceholderMesh->GetMaterial(0);
	if (!ParentMat)
	{
		return;
	}

	UMaterialInstanceDynamic* MID = PlaceholderMesh->CreateDynamicMaterialInstance(0, ParentMat);
	if (!MID)
	{
		return;
	}

	// Wild = red cube; Farm Combat = green; Farm Collect = yellow (easy to see T toggle)
	FLinearColor Color(0.95f, 0.05f, 0.05f, 1.f);
	if (Team == ERTSTeam::Farm)
	{
		Color = (WorkMode == EUnitWorkMode::Collect)
			? FLinearColor(0.95f, 0.85f, 0.05f, 1.f)
			: FLinearColor(0.05f, 0.95f, 0.15f, 1.f);
	}
	MID->SetVectorParameterValue(TEXT("Color"), Color);
	MID->SetVectorParameterValue(TEXT("BaseColor"), Color);
}

void ARTSUnitBase::SetSelectedHighlight(bool bSelected)
{
	// Animal BP mesh: keep authored scale; only placeholder needs size feedback.
	if (Mesh && Mesh->SkeletalMesh != nullptr)
	{
		return;
	}

	if (!PlaceholderMesh)
	{
		return;
	}
	const FVector BaseScale = (Team == ERTSTeam::Wild)
		? FVector(1.0f, 1.0f, 1.2f)
		: FVector(1.1f, 1.1f, 1.4f);
	PlaceholderMesh->SetRelativeScale3D(bSelected ? BaseScale * 1.35f : BaseScale);
}

void ARTSUnitBase::EnterState(ERTSUnitState NewState)
{
	const ERTSUnitState OldState = UnitState;
	UnitState = NewState;

	if (NewState == ERTSUnitState::Collecting)
	{
		PlayCollectAnimation();
	}
	else if (OldState == ERTSUnitState::Collecting && NewState != ERTSUnitState::Dead)
	{
		RestoreWalkAnimation();
	}
}

void ARTSUnitBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsAlive() || !AssignedLane)
	{
		return;
	}

	AttackCooldownRemaining = FMath::Max(0.f, AttackCooldownRemaining - DeltaSeconds);
	UpdatePackRushCooldown();

	switch (UnitState)
	{
	case ERTSUnitState::Moving:
	case ERTSUnitState::Guarding:
		if (WorkMode == EUnitWorkMode::Collect && Team == ERTSTeam::Farm)
		{
			TickMovement(DeltaSeconds);
		}
		else
		{
			if (AActor* Enemy = FindClosestEnemy())
			{
				CurrentTarget = Enemy;
				EnterState(ERTSUnitState::Combat);
			}
			else if (Team == ERTSTeam::Wild && IsInAttackRangeOfCore())
			{
				CurrentTarget = FindCoreBuilding();
				if (CurrentTarget)
				{
					EnterState(ERTSUnitState::Combat);
				}
			}
			else if (UnitState == ERTSUnitState::Moving)
			{
				TickMovement(DeltaSeconds);
			}
		}
		break;
	case ERTSUnitState::Combat:
		TickCombat(DeltaSeconds);
		break;
	case ERTSUnitState::Collecting:
		TickCollecting(DeltaSeconds);
		break;
	default:
		break;
	}
}

void ARTSUnitBase::ResolveCombatSpacing(float DesiredDelta)
{
	if (!AssignedLane || FMath::IsNearlyZero(DesiredDelta))
	{
		return;
	}

	const float LaneLen = AssignedLane->GetSplineLength();
	if (LaneLen < 50.f)
	{
		return;
	}

	float Candidate = DistanceAlongSpline + DesiredDelta;

	// Only keep distance from allies ahead in our movement direction.
	// Old Abs() check froze units stacked at spawn (distance 0).
	if (WorkMode == EUnitWorkMode::Combat)
	{
		TArray<AActor*> Units;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARTSUnitBase::StaticClass(), Units);

		for (AActor* Actor : Units)
		{
			ARTSUnitBase* Other = Cast<ARTSUnitBase>(Actor);
			if (!Other || Other == this || !Other->IsAlive() || Other->AssignedLane != AssignedLane)
			{
				continue;
			}
			if (Other->WorkMode != EUnitWorkMode::Combat || Other->Team != Team)
			{
				continue;
			}

			if (DesiredDelta > 0.f && Other->DistanceAlongSpline > DistanceAlongSpline + 1.f)
			{
				Candidate = FMath::Min(Candidate, Other->DistanceAlongSpline - CombatSpacing);
			}
			else if (DesiredDelta < 0.f && Other->DistanceAlongSpline < DistanceAlongSpline - 1.f)
			{
				Candidate = FMath::Max(Candidate, Other->DistanceAlongSpline + CombatSpacing);
			}
		}
	}

	// Never move backwards due to spacing when we intend to advance.
	if (DesiredDelta > 0.f)
	{
		Candidate = FMath::Max(Candidate, DistanceAlongSpline);
	}
	else if (DesiredDelta < 0.f)
	{
		Candidate = FMath::Min(Candidate, DistanceAlongSpline);
	}

	DistanceAlongSpline = FMath::Clamp(Candidate, 0.f, LaneLen);
}

void ARTSUnitBase::ApplySplineTransform(bool bApplyTravelFacing)
{
	if (!AssignedLane)
	{
		return;
	}
	const FVector Loc = AssignedLane->GetLocationAtDistance(DistanceAlongSpline) + FVector(0.f, 0.f, 60.f);
	SetActorLocation(Loc, false, nullptr, ETeleportType::TeleportPhysics);

	if (!bApplyTravelFacing)
	{
		return;
	}

	FRotator Rot = AssignedLane->GetRotationAtDistance(DistanceAlongSpline);
	if (SplineTravelDir < 0.f)
	{
		Rot.Yaw += 180.f;
	}
	SetActorRotation(Rot);
}

void ARTSUnitBase::FaceWorldLocation(const FVector& WorldLocation)
{
	FVector ToTarget = WorldLocation - GetActorLocation();
	ToTarget.Z = 0.f;
	if (!ToTarget.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
	}
}

void ARTSUnitBase::TickMovement(float DeltaSeconds)
{
	const float LaneLen = AssignedLane->GetSplineLength();
	if (LaneLen < 50.f)
	{
		// Broken / zero-length spline — do not freeze in Guarding.
		return;
	}

	if (WorkMode == EUnitWorkMode::Collect && Team == ERTSTeam::Farm)
	{
		if (!TargetResource)
		{
			TargetResource = AssignedLane->FindNearestResourceNode(DistanceAlongSpline);
			if (!TargetResource)
			{
				return;
			}
		}

		const float Goal = bMovingTowardResource ? TargetResource->SplineDistance : 0.f;
		const float Dir = FMath::Sign(Goal - DistanceAlongSpline);
		SplineTravelDir = FMath::IsNearlyZero(Dir) ? SplineTravelDir : Dir;
		if (FMath::IsNearlyZero(Dir))
		{
			if (bMovingTowardResource)
			{
				CollectTimer = 0.f;
				EnterState(ERTSUnitState::Collecting);
			}
			else
			{
				bMovingTowardResource = true;
			}
			ApplySplineTransform();
			return;
		}

		if (FMath::Abs(Goal - DistanceAlongSpline) <= TargetResource->ArrivalTolerance)
		{
			DistanceAlongSpline = Goal;
			if (bMovingTowardResource)
			{
				CollectTimer = 0.f;
				EnterState(ERTSUnitState::Collecting);
			}
			else
			{
				bMovingTowardResource = true;
			}
			ApplySplineTransform();
			return;
		}

		DistanceAlongSpline += Dir * Stats.MoveSpeed * DeltaSeconds;
		DistanceAlongSpline = FMath::Clamp(DistanceAlongSpline, 0.f, LaneLen);
	}
	else
	{
		if (Team == ERTSTeam::Wild)
		{
			ARTSBaseBuilding* Core = FindCoreBuilding();
			const float CoreDist = GetCoreBuildingLaneDistance();

			if (Core && IsInAttackRangeOfCore())
			{
				DistanceAlongSpline = FMath::Clamp(CoreDist, 0.f, LaneLen);
				CurrentTarget = Core;
				EnterState(ERTSUnitState::Combat);
			}
			else
			{
				SplineTravelDir = -1.f;
				ResolveCombatSpacing(-Stats.MoveSpeed * DeltaSeconds);
				if (DistanceAlongSpline < CoreDist)
				{
					DistanceAlongSpline = CoreDist;
				}
			}
		}
		else
		{
			// Farm combat: advance toward enemy end (increasing distance)
			const float Dir = 1.f;
			SplineTravelDir = Dir;
			ResolveCombatSpacing(Dir * Stats.MoveSpeed * DeltaSeconds);

			if (DistanceAlongSpline >= LaneLen - 5.f)
			{
				DistanceAlongSpline = LaneLen;
				bReachedLaneEnd = true;
				SplineTravelDir = -1.f;
				EnterState(ERTSUnitState::Guarding);
			}
		}
	}

	ApplySplineTransform();
}

void ARTSUnitBase::TickCollecting(float DeltaSeconds)
{
	// Take damage may kill us; Collect mode never leaves to Combat.
	if (AActor* Enemy = FindClosestEnemy())
	{
		// Enemies can still choose us as target; we do not retaliate.
		(void)Enemy;
	}

	CollectTimer += DeltaSeconds;
	if (CollectTimer >= (TargetResource ? TargetResource->CollectInterval : 3.f))
	{
		CollectTimer = 0.f;
		const float Amount = (TargetResource ? TargetResource->BaseFodderPerTick : 5.f) * Stats.CollectEfficiencyMultiplier;
		if (ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GM->AddFodder(FMath::RoundToInt(Amount));
		}
		bMovingTowardResource = false;
		EnterState(ERTSUnitState::Moving);
	}
}

void ARTSUnitBase::TickCombat(float DeltaSeconds)
{
	if (!CurrentTarget || !CanAttackTarget(CurrentTarget))
	{
		CurrentTarget = FindClosestEnemy();
		if (!CurrentTarget && Team == ERTSTeam::Wild && IsInAttackRangeOfCore())
		{
			CurrentTarget = FindCoreBuilding();
		}
		if (!CurrentTarget)
		{
			EnterState(bReachedLaneEnd ? ERTSUnitState::Guarding : ERTSUnitState::Moving);
			return;
		}
	}

	if (ARTSBaseBuilding* Building = Cast<ARTSBaseBuilding>(CurrentTarget))
	{
		if (!IsInAttackRangeOfCore())
		{
			CurrentTarget = nullptr;
			EnterState(ERTSUnitState::Moving);
			return;
		}

		ApplySplineTransform(false);
		FaceWorldLocation(Building->GetActorLocation());

		if (AttackCooldownRemaining > 0.f)
		{
			return;
		}

		const float Damage = ComputeOutgoingDamage();
		TryAmbushOnFirstHit();
		AttackCooldownRemaining = EffectiveAttackCooldown;
		PlayAttackAnimation();
		Building->ReceiveDamage(Damage, this);
		return;
	}

	const float Dist = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
	if (Dist > AttackRange)
	{
		// Close distance along lane toward target if it's a unit on same lane
		if (ARTSUnitBase* TargetUnit = Cast<ARTSUnitBase>(CurrentTarget))
		{
			if (TargetUnit->AssignedLane == AssignedLane)
			{
				const float Dir = FMath::Sign(TargetUnit->DistanceAlongSpline - DistanceAlongSpline);
				if (!FMath::IsNearlyZero(Dir))
				{
					SplineTravelDir = Dir;
				}
				ResolveCombatSpacing(Dir * Stats.MoveSpeed * DeltaSeconds);
			}
		}
		else if (Team == ERTSTeam::Wild)
		{
			SplineTravelDir = -1.f;
			ResolveCombatSpacing(-Stats.MoveSpeed * DeltaSeconds);
		}
		ApplySplineTransform(); // keep +Z ground offset (bare spline Z sinks meshes)
		return;
	}

	// In melee range: face the target along the lane
	if (ARTSUnitBase* TargetUnit = Cast<ARTSUnitBase>(CurrentTarget))
	{
		if (TargetUnit->AssignedLane == AssignedLane)
		{
			const float Dir = FMath::Sign(TargetUnit->DistanceAlongSpline - DistanceAlongSpline);
			if (!FMath::IsNearlyZero(Dir))
			{
				SplineTravelDir = Dir;
			}
		}
	}

	ApplySplineTransform();

	if (AttackCooldownRemaining > 0.f)
	{
		return;
	}

	const float Damage = ComputeOutgoingDamage();
	TryAmbushOnFirstHit();
	AttackCooldownRemaining = EffectiveAttackCooldown;
	PlayAttackAnimation();

	if (ARTSUnitBase* TargetUnit = Cast<ARTSUnitBase>(CurrentTarget))
	{
		TargetUnit->TakeDamageAmount(Damage, this);
	}
}

void ARTSUnitBase::CacheWalkAnimFromMesh()
{
	if (WalkAnim || !Mesh)
	{
		return;
	}
	if (UAnimationAsset* Current = Mesh->AnimationData.AnimToPlay)
	{
		WalkAnim = Cast<UAnimSequence>(Current);
	}
}

void ARTSUnitBase::PlayWalkAnimation()
{
	if (!Mesh || !WalkAnim || bPlayingAttackAnim || UnitState == ERTSUnitState::Dead)
	{
		return;
	}
	if (UnitState == ERTSUnitState::Collecting)
	{
		PlayCollectAnimation();
		return;
	}
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Mesh->PlayAnimation(WalkAnim, true);
}

void ARTSUnitBase::PlayAttackAnimation()
{
	if (!Mesh || !AttackAnim)
	{
		return;
	}

	CacheWalkAnimFromMesh();
	bPlayingAttackAnim = true;
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Mesh->PlayAnimation(AttackAnim, false);

	float Duration = AttackAnim->SequenceLength;
	if (Duration < 0.15f)
	{
		Duration = 0.15f;
	}
	// Don't let a long attack clip block the next hit for too long.
	Duration = FMath::Min(Duration, FMath::Max(0.2f, EffectiveAttackCooldown * 0.9f));

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackAnimTimer);
		World->GetTimerManager().SetTimer(
			AttackAnimTimer,
			this,
			&ARTSUnitBase::RestoreWalkAnimation,
			Duration,
			false);
	}
}

void ARTSUnitBase::PlayCollectAnimation()
{
	if (!Mesh || !CollectAnim)
	{
		return;
	}
	CacheWalkAnimFromMesh();
	bPlayingAttackAnim = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackAnimTimer);
	}
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Mesh->PlayAnimation(CollectAnim, true);
}

void ARTSUnitBase::RestoreWalkAnimation()
{
	bPlayingAttackAnim = false;
	if (!IsAlive() || UnitState == ERTSUnitState::Dead)
	{
		return;
	}
	if (UnitState == ERTSUnitState::Collecting)
	{
		PlayCollectAnimation();
		return;
	}
	PlayWalkAnimation();
}

void ARTSUnitBase::PlayDeathAnimation()
{
	if (!Mesh || !DeathAnim)
	{
		return;
	}

	bPlayingAttackAnim = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackAnimTimer);
	}

	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Mesh->PlayAnimation(DeathAnim, false);
}

void ARTSUnitBase::Die()
{
	if (UnitState == ERTSUnitState::Dead)
	{
		return;
	}

	DestroyBossRing();

	EnterState(ERTSUnitState::Dead);
	CurrentTarget = nullptr;
	TargetResource = nullptr;

	if (ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->NotifyUnitDied(this);
	}

	SetActorEnableCollision(false);
	PrimaryActorTick.bCanEverTick = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackAnimTimer);
		World->GetTimerManager().ClearTimer(DeathAnimTimer);
	}

	float Lifespan = 1.5f;
	if (DeathAnim && Mesh)
	{
		PlayDeathAnimation();
		Lifespan = FMath::Max(1.2f, DeathAnim->SequenceLength + 0.8f);
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				DeathAnimTimer,
				this,
				&ARTSUnitBase::FinishDeath,
				DeathAnim->SequenceLength + 0.05f,
				false);
		}
	}
	else
	{
		FinishDeath();
	}

	SetLifeSpan(Lifespan);
}

void ARTSUnitBase::FinishDeath()
{
	// Hold last death pose briefly; SetLifeSpan still removes the actor.
	if (Mesh && DeathAnim)
	{
		Mesh->Stop();
		Mesh->SetPosition(DeathAnim->SequenceLength, false);
	}
	else
	{
		SetActorHiddenInGame(true);
	}
}

float ARTSUnitBase::ComputeOutgoingDamage()
{
	float Damage = Stats.AttackPower;
	if (Stats.bAmbushTrait && !bHasUsedAmbush)
	{
		Damage *= 1.2f;
	}
	return Damage;
}

void ARTSUnitBase::TryAmbushOnFirstHit()
{
	if (Stats.bAmbushTrait && !bHasUsedAmbush)
	{
		bHasUsedAmbush = true;
	}
}

void ARTSUnitBase::UpdatePackRushCooldown()
{
	EffectiveAttackCooldown = AttackCooldown;
	if (Stats.bPackRushTrait && CurrentHealth / Stats.MaxHealth < 0.3f)
	{
		EffectiveAttackCooldown = AttackCooldown / 1.5f;
	}
}

AActor* ARTSUnitBase::FindClosestEnemy() const
{
	TArray<AActor*> Units;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARTSUnitBase::StaticClass(), Units);

	AActor* Best = nullptr;
	float BestDistSq = PerceptionRadius * PerceptionRadius;

	for (AActor* Actor : Units)
	{
		ARTSUnitBase* Other = Cast<ARTSUnitBase>(Actor);
		if (!Other || Other == this || !Other->IsAlive() || Other->Team == Team)
		{
			continue;
		}
		if (AssignedLane && Other->AssignedLane && Other->AssignedLane != AssignedLane)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(GetActorLocation(), Other->GetActorLocation());
		if (DistSq <= BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Other;
		}
	}
	return Best;
}

ARTSBaseBuilding* ARTSUnitBase::FindCoreBuilding() const
{
	TArray<AActor*> Buildings;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARTSBaseBuilding::StaticClass(), Buildings);
	ARTSBaseBuilding* Best = nullptr;
	// Wild units march far — use generous seek radius for the coop.
	const float SeekRadius = (Team == ERTSTeam::Wild) ? 3000.f : PerceptionRadius * 2.f;
	float BestDistSq = SeekRadius * SeekRadius;
	for (AActor* Actor : Buildings)
	{
		ARTSBaseBuilding* Building = Cast<ARTSBaseBuilding>(Actor);
		if (!Building || !Building->IsAlive() || !Building->bIsCoreBuilding)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(GetActorLocation(), Building->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Building;
		}
	}
	return Best;
}

float ARTSUnitBase::GetCoreBuildingLaneDistance() const
{
	ARTSBaseBuilding* Core = FindCoreBuilding();
	if (!Core || !AssignedLane)
	{
		return 0.f;
	}
	return AssignedLane->GetDistanceForWorldLocation(Core->GetActorLocation());
}

bool ARTSUnitBase::IsInAttackRangeOfCore() const
{
	if (!FindCoreBuilding() || !AssignedLane)
	{
		return false;
	}
	const float CoreDist = GetCoreBuildingLaneDistance();
	// Coop is beside the lane: attack once we reach its perpendicular foot on the spline.
	constexpr float LaneTolerance = 40.f;
	return FMath::Abs(DistanceAlongSpline - CoreDist) <= LaneTolerance;
}

bool ARTSUnitBase::CanAttackTarget(AActor* Target) const
{
	if (!Target)
	{
		return false;
	}
	if (ARTSUnitBase* Unit = Cast<ARTSUnitBase>(Target))
	{
		return Unit->IsAlive();
	}
	if (ARTSBaseBuilding* Building = Cast<ARTSBaseBuilding>(Target))
	{
		return Building->IsAlive();
	}
	return false;
}

void ARTSUnitBase::TakeDamageAmount(float Amount, AActor* DamageCauser)
{
	if (!IsAlive())
	{
		return;
	}

	CurrentHealth -= Amount;
	TryFoxBossSummon();

	if (CurrentHealth <= 0.f)
	{
		CurrentHealth = 0.f;
		Die();
	}
}

void ARTSUnitBase::TryFoxBossSummon()
{
	if (UnitType != ERTSUnitType::FoxBoss || bFoxBossSummonTriggered || !AssignedLane)
	{
		return;
	}
	if (CurrentHealth > 80.f)
	{
		return;
	}

	bFoxBossSummonTriggered = true;
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));
	TSubclassOf<ARTSUnitBase> FoxClass = GM
		? GM->ResolveWildUnitClass(ERTSUnitType::Fox)
		: ARTSUnitBase::StaticClass();

	for (int32 i = 0; i < 3; ++i)
	{
		const float SpawnDist = FMath::Clamp(DistanceAlongSpline + (i - 1) * 60.f, 0.f, AssignedLane->GetSplineLength());
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ARTSUnitBase* Fox = World->SpawnActor<ARTSUnitBase>(FoxClass, AssignedLane->GetLocationAtDistance(SpawnDist), FRotator::ZeroRotator, Params);
		if (Fox)
		{
			Fox->InitializeUnit(ERTSUnitType::Fox, AssignedLane, SpawnDist, EUnitWorkMode::Combat);
			if (GM)
			{
				GM->NotifyUnitSpawned(Fox);
			}
		}
	}
}
