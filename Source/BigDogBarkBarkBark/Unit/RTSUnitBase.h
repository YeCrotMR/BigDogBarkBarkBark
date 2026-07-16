// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RTSUnitTypes.h"
#include "RTSUnitBase.generated.h"

class ARTSLaneSpline;
class ARTSResourceNode;
class ARTSBaseBuilding;
class UCapsuleComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UAnimSequence;

UCLASS()
class BIGDOGBARKBARKBARK_API ARTSUnitBase : public APawn
{
	GENERATED_BODY()

public:
	ARTSUnitBase();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Unit")
	UCapsuleComponent* Capsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Unit")
	USkeletalMeshComponent* Mesh;

	/** Visible placeholder until animal meshes are assigned in BP. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Unit")
	UStaticMeshComponent* PlaceholderMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Unit")
	ERTSUnitType UnitType = ERTSUnitType::Sheep;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Unit")
	FRTSUnitStats Stats;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Unit")
	ERTSTeam Team = ERTSTeam::Farm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Unit")
	EUnitWorkMode WorkMode = EUnitWorkMode::Combat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Unit")
	ERTSUnitState UnitState = ERTSUnitState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Unit")
	float CurrentHealth = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Lane")
	ARTSLaneSpline* AssignedLane = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Lane")
	float DistanceAlongSpline = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Combat")
	float PerceptionRadius = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Combat")
	float AttackRange = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Combat")
	float AttackCooldown = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Lane")
	float CombatSpacing = 80.f;

	/** Looping locomotion for Mesh (Use Animation Asset). If empty, uses Mesh's Anim to Play. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Anim")
	UAnimSequence* WalkAnim = nullptr;

	/** One-shot attack clip played each melee hit. Must share Mesh skeleton. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Anim")
	UAnimSequence* AttackAnim = nullptr;

	/** Looping collect / eat clip while UnitState == Collecting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Anim")
	UAnimSequence* CollectAnim = nullptr;

	/** One-shot death / fall-down clip. Must share Mesh skeleton. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Anim")
	UAnimSequence* DeathAnim = nullptr;

	UFUNCTION(BlueprintCallable, Category = "RTS|Unit")
	void InitializeUnit(ERTSUnitType InType, ARTSLaneSpline* Lane, float StartDistance, EUnitWorkMode InMode);

	UFUNCTION(BlueprintCallable, Category = "RTS|Unit")
	void SetWorkMode(EUnitWorkMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "RTS|Unit")
	void TakeDamageAmount(float Amount, AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category = "RTS|Unit")
	bool IsAlive() const { return UnitState != ERTSUnitState::Dead && CurrentHealth > 0.f; }

	UFUNCTION(BlueprintCallable, Category = "RTS|Unit")
	bool IsFarmUnit() const { return Team == ERTSTeam::Farm; }

	UFUNCTION(BlueprintCallable, Category = "RTS|Unit")
	void ApplyTeamAppearance();

	UFUNCTION(BlueprintCallable, Category = "RTS|Unit")
	void SetSelectedHighlight(bool bSelected);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void ApplyWorkModeCollision();
	void TickMovement(float DeltaSeconds);
	void TickCombat(float DeltaSeconds);
	void TickCollecting(float DeltaSeconds);
	void EnterState(ERTSUnitState NewState);
	void Die();

	AActor* FindClosestEnemy() const;
	ARTSBaseBuilding* FindCoreBuilding() const;
	float GetCoreBuildingLaneDistance() const;
	bool IsInAttackRangeOfCore() const;
	bool CanAttackTarget(AActor* Target) const;
	float ComputeOutgoingDamage();
	void TryAmbushOnFirstHit();
	void UpdatePackRushCooldown();
	void TryFoxBossSummon();

	void ResolveCombatSpacing(float DesiredDelta);
	void ApplySplineTransform(bool bApplyTravelFacing = true);
	void FaceWorldLocation(const FVector& WorldLocation);
	void CacheWalkAnimFromMesh();
	void PlayWalkAnimation();
	void PlayAttackAnimation();
	void PlayCollectAnimation();
	void PlayDeathAnimation();
	void RestoreWalkAnimation();
	void FinishDeath();

	UPROPERTY()
	UStaticMesh* FarmPlaceholderMesh = nullptr;

	UPROPERTY()
	UStaticMesh* WildPlaceholderMesh = nullptr;

	UPROPERTY()
	UMaterialInterface* PlaceholderMaterial = nullptr;

	UPROPERTY()
	AActor* CurrentTarget = nullptr;

	UPROPERTY()
	ARTSResourceNode* TargetResource = nullptr;

	float AttackCooldownRemaining = 0.f;
	float CollectTimer = 0.f;
	bool bMovingTowardResource = true;
	bool bReachedLaneEnd = false;
	bool bHasUsedAmbush = false;
	bool bFoxBossSummonTriggered = false;
	float EffectiveAttackCooldown = 1.5f;
	bool bPlayingAttackAnim = false;
	FTimerHandle AttackAnimTimer;
	FTimerHandle DeathAnimTimer;
	/** +1 = toward increasing spline distance, -1 = toward base/spawn end. */
	float SplineTravelDir = 1.f;
};
