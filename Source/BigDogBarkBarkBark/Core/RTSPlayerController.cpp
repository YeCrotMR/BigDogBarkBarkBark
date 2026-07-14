// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSPlayerController.h"
#include "RTSGameMode.h"
#include "RTSUnitBase.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"
#include "Engine/EngineTypes.h"

ARTSPlayerController::ARTSPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
	HUDWidgetClass = nullptr;
}

void ARTSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = true;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow,
			TEXT("Recruit 1-4 | Left-click select (or press T without select = nearest) | T switch Combat/Collect"));
	}
}

void ARTSPlayerController::CreateHUD()
{
}

void ARTSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Only BindAction for keys already in DefaultInput.ini.
	// Binding the same key again via BindKey would fire the handler twice
	// (T would Combat->Collect->Combat in one press = looks like no change).
	InputComponent->BindAction("SelectClick", IE_Pressed, this, &ARTSPlayerController::OnLeftClick);
	InputComponent->BindAction("RecruitRabbit", IE_Pressed, this, &ARTSPlayerController::HotkeyRecruitRabbit);
	InputComponent->BindAction("RecruitChicken", IE_Pressed, this, &ARTSPlayerController::HotkeyRecruitChicken);
	InputComponent->BindAction("RecruitSheep", IE_Pressed, this, &ARTSPlayerController::HotkeyRecruitSheep);
	InputComponent->BindAction("RecruitPig", IE_Pressed, this, &ARTSPlayerController::HotkeyRecruitPig);
	InputComponent->BindAction("SelectLane0", IE_Pressed, this, &ARTSPlayerController::HotkeyLane0);
	InputComponent->BindAction("SelectLane1", IE_Pressed, this, &ARTSPlayerController::HotkeyLane1);
	InputComponent->BindAction("ToggleWorkMode", IE_Pressed, this, &ARTSPlayerController::HotkeyToggleMode);

	// Extra aliases only (not in ActionMappings)
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ARTSPlayerController::HotkeyToggleMode);
	InputComponent->BindKey(EKeys::NumPadOne, IE_Pressed, this, &ARTSPlayerController::HotkeyRecruitRabbit);
	InputComponent->BindKey(EKeys::NumPadTwo, IE_Pressed, this, &ARTSPlayerController::HotkeyRecruitChicken);
	InputComponent->BindKey(EKeys::NumPadThree, IE_Pressed, this, &ARTSPlayerController::HotkeyRecruitSheep);
	InputComponent->BindKey(EKeys::NumPadFour, IE_Pressed, this, &ARTSPlayerController::HotkeyRecruitPig);
}

void ARTSPlayerController::OnLeftClick()
{
	SelectUnitUnderCursor();
}

void ARTSPlayerController::SelectUnitUnderCursor()
{
	if (SelectedUnit)
	{
		SelectedUnit->SetSelectedHighlight(false);
	}
	SelectedUnit = nullptr;

	FVector WorldOrigin;
	FVector WorldDir;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDir))
	{
		return;
	}

	const FVector TraceEnd = WorldOrigin + WorldDir * 100000.f;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RTSSelect), true);
	Params.bTraceComplex = false;

	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, WorldOrigin, TraceEnd, ECC_Visibility, Params);
	if (!bHit)
	{
		bHit = GetWorld()->LineTraceSingleByChannel(Hit, WorldOrigin, TraceEnd, ECC_Camera, Params);
	}
	if (!bHit)
	{
		bHit = GetWorld()->LineTraceSingleByChannel(Hit, WorldOrigin, TraceEnd, ECC_WorldDynamic, Params);
	}

	if (bHit)
	{
		SelectedUnit = Cast<ARTSUnitBase>(Hit.GetActor());
	}

	// Sphere fallback around hit / far along ray — click near unit still works
	if (!SelectedUnit || !SelectedUnit->IsFarmUnit() || !SelectedUnit->IsAlive())
	{
		SelectedUnit = nullptr;
		const FVector Probe = bHit ? Hit.ImpactPoint : (WorldOrigin + WorldDir * 3000.f);
		TArray<AActor*> Units;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARTSUnitBase::StaticClass(), Units);

		float BestDistSq = 250.f * 250.f;
		for (AActor* Actor : Units)
		{
			ARTSUnitBase* Unit = Cast<ARTSUnitBase>(Actor);
			if (!Unit || !Unit->IsAlive() || !Unit->IsFarmUnit())
			{
				continue;
			}
			const float DistSq = FVector::DistSquared(Unit->GetActorLocation(), Probe);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				SelectedUnit = Unit;
			}
		}
	}

	if (SelectedUnit && SelectedUnit->IsFarmUnit() && SelectedUnit->IsAlive())
	{
		SelectedUnit->SetSelectedHighlight(true);
		if (GEngine)
		{
			const FString Mode = SelectedUnit->WorkMode == EUnitWorkMode::Collect ? TEXT("Collect") : TEXT("Combat");
			GEngine->AddOnScreenDebugMessage(4, 2.f, FColor::Cyan,
				FString::Printf(TEXT("Selected farm unit | mode=%s | press T to toggle"), *Mode));
		}
	}
	else
	{
		SelectedUnit = nullptr;
	}
}

ARTSUnitBase* ARTSPlayerController::FindNearestFarmUnit() const
{
	APawn* Cam = GetPawn();
	const FVector Origin = Cam ? Cam->GetActorLocation() : FVector::ZeroVector;

	TArray<AActor*> Units;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARTSUnitBase::StaticClass(), Units);

	ARTSUnitBase* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (AActor* Actor : Units)
	{
		ARTSUnitBase* Unit = Cast<ARTSUnitBase>(Actor);
		if (!Unit || !Unit->IsAlive() || !Unit->IsFarmUnit())
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Unit->GetActorLocation(), Origin);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Unit;
		}
	}
	return Best;
}

void ARTSPlayerController::SetSelectedLane(int32 LaneIndex)
{
	SelectedLaneIndex = FMath::Max(0, LaneIndex);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 2.f, FColor::Cyan,
			FString::Printf(TEXT("Selected lane: %d"), SelectedLaneIndex));
	}
}

void ARTSPlayerController::RecruitSelectedType()
{
	RecruitType(PendingRecruitType);
}

void ARTSPlayerController::RecruitType(ERTSUnitType Type)
{
	PendingRecruitType = Type;
	ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
				TEXT("Recruit failed: GameMode is not RTSGameMode"));
		}
		return;
	}

	if (SelectedUnit)
	{
		SelectedUnit->SetSelectedHighlight(false);
	}

	ARTSUnitBase* Unit = GM->RecruitUnit(Type, SelectedLaneIndex, EUnitWorkMode::Combat);
	if (Unit)
	{
		SelectedUnit = Unit;
		SelectedUnit->SetSelectedHighlight(true);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(2, 2.f, FColor::Green,
				TEXT("Recruited & selected | press T for Collect / Combat"));
		}
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2, 2.f, FColor::Orange,
			TEXT("Recruit failed (fodder / no lane?)"));
	}
}

void ARTSPlayerController::SetSelectedUnitWorkMode(EUnitWorkMode Mode)
{
	if (SelectedUnit && SelectedUnit->IsAlive() && SelectedUnit->IsFarmUnit())
	{
		SelectedUnit->SetWorkMode(Mode);
	}
}

void ARTSPlayerController::ToggleSelectedUnitWorkMode()
{
	if (!SelectedUnit || !SelectedUnit->IsAlive() || !SelectedUnit->IsFarmUnit())
	{
		// Convenience: no selection → pick nearest farm unit then toggle
		if (SelectedUnit)
		{
			SelectedUnit->SetSelectedHighlight(false);
		}
		SelectedUnit = FindNearestFarmUnit();
		if (SelectedUnit)
		{
			SelectedUnit->SetSelectedHighlight(true);
		}
	}

	if (!SelectedUnit || !SelectedUnit->IsAlive())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(3, 2.f, FColor::Orange,
				TEXT("No farm unit to toggle — recruit with 1-4 first"));
		}
		return;
	}

	const EUnitWorkMode Next = SelectedUnit->WorkMode == EUnitWorkMode::Combat
		? EUnitWorkMode::Collect
		: EUnitWorkMode::Combat;
	SelectedUnit->SetWorkMode(Next);

	if (GEngine)
	{
		const FString ModeName = Next == EUnitWorkMode::Collect ? TEXT("COLLECT") : TEXT("COMBAT");
		GEngine->AddOnScreenDebugMessage(3, 3.f, FColor::Yellow,
			FString::Printf(TEXT("Work mode -> %s"), *ModeName));
	}
}

void ARTSPlayerController::HotkeyRecruitRabbit() { RecruitType(ERTSUnitType::Rabbit); }
void ARTSPlayerController::HotkeyRecruitChicken() { RecruitType(ERTSUnitType::Chicken); }
void ARTSPlayerController::HotkeyRecruitSheep() { RecruitType(ERTSUnitType::Sheep); }
void ARTSPlayerController::HotkeyRecruitPig() { RecruitType(ERTSUnitType::Pig); }
void ARTSPlayerController::HotkeyLane0() { SetSelectedLane(0); }
void ARTSPlayerController::HotkeyLane1() { SetSelectedLane(1); }
void ARTSPlayerController::HotkeyToggleMode() { ToggleSelectedUnitWorkMode(); }
