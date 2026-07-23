// Copyright Epic Games, Inc. All Rights Reserved.

#include "RTSPlayerController.h"
#include "RTSGameMode.h"
#include "RTSGameHUD.h"
#include "RTSUnitBase.h"
#include "RTSLaneSpline.h"
#include "RTSUnitModeRing.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"
#include "CollisionQueryParams.h"
#include "Engine/EngineTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/WidgetPath.h"

ARTSPlayerController::ARTSPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
	HUDWidgetClass = URTSGameHUD::StaticClass();
}

void ARTSPlayerController::BeginPlay()
{
	Super::BeginPlay();
	CreateHUD();

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ARTSPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bPlacementPending)
	{
		UpdatePlacementLaneHighlight();
	}

	if (SelectedUnit && (!SelectedUnit->IsAlive() || !SelectedUnit->IsFarmUnit()))
	{
		ClearSelectedUnit();
	}
	else if (SelectedUnit && !ModeRing.IsValid())
	{
		ShowModeRingForSelected();
	}
}

void ARTSPlayerController::CreateHUD()
{
	if (HUDWidget)
	{
		return;
	}

	TSubclassOf<URTSGameHUD> ClassToSpawn = HUDWidgetClass;
	if (!ClassToSpawn)
	{
		if (ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			ClassToSpawn = GM->HUDWidgetClass;
		}
	}
	if (!ClassToSpawn)
	{
		ClassToSpawn = URTSGameHUD::StaticClass();
	}

	HUDWidget = CreateWidget<URTSGameHUD>(this, ClassToSpawn);
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		HUDWidget->AddToViewport(100);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(20, 5.f, FColor::Green, TEXT("RTS HUD loaded"));
		}
		if (ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GM->NotifyHUDReady();
		}
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(20, 8.f, FColor::Red, TEXT("RTS HUD failed to create"));
	}
}

void ARTSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAction("SelectClick", IE_Pressed, this, &ARTSPlayerController::OnLeftClick);
	InputComponent->BindAction("RecruitRabbit", IE_Pressed, this, &ARTSPlayerController::HotkeyRecruitRabbit);
	InputComponent->BindAction("RecruitChicken", IE_Pressed, this, &ARTSPlayerController::HotkeyRecruitChicken);
	InputComponent->BindAction("RecruitSheep", IE_Pressed, this, &ARTSPlayerController::HotkeyRecruitSheep);
	InputComponent->BindAction("RecruitPig", IE_Pressed, this, &ARTSPlayerController::HotkeyRecruitPig);
	InputComponent->BindAction("SelectLane0", IE_Pressed, this, &ARTSPlayerController::HotkeyLane0);
	InputComponent->BindAction("SelectLane1", IE_Pressed, this, &ARTSPlayerController::HotkeyLane1);
	InputComponent->BindAction("ToggleWorkMode", IE_Pressed, this, &ARTSPlayerController::HotkeyToggleMode);

	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ARTSPlayerController::HotkeyToggleMode);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ARTSPlayerController::OnCancelPlacementKey);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ARTSPlayerController::OnRightClick);
	InputComponent->BindKey(EKeys::NumPadOne, IE_Pressed, this, &ARTSPlayerController::HotkeyRecruitRabbit);
	InputComponent->BindKey(EKeys::NumPadTwo, IE_Pressed, this, &ARTSPlayerController::HotkeyRecruitChicken);
	InputComponent->BindKey(EKeys::NumPadThree, IE_Pressed, this, &ARTSPlayerController::HotkeyRecruitSheep);
	InputComponent->BindKey(EKeys::NumPadFour, IE_Pressed, this, &ARTSPlayerController::HotkeyRecruitPig);

	InputComponent->BindKey(EKeys::F1, IE_Pressed, this, &ARTSPlayerController::HotkeyUpgradeRabbit);
	InputComponent->BindKey(EKeys::F2, IE_Pressed, this, &ARTSPlayerController::HotkeyUpgradeChicken);
	InputComponent->BindKey(EKeys::F3, IE_Pressed, this, &ARTSPlayerController::HotkeyUpgradeSheep);
	InputComponent->BindKey(EKeys::F4, IE_Pressed, this, &ARTSPlayerController::HotkeyUpgradePig);
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ARTSPlayerController::OnAdvanceDialogueKey);
}

void ARTSPlayerController::OnLeftClick()
{
	if (HUDWidget && HUDWidget->IsDialogueActive())
	{
		HUDWidget->AdvanceDialogue();
		return;
	}
	if (IsPointerOverInteractiveUI())
	{
		return;
	}
	if (bPlacementPending)
	{
		HandleWorldClickForPlacement();
		return;
	}
	SelectUnitUnderCursor();
}

void ARTSPlayerController::OnAdvanceDialogueKey()
{
	if (HUDWidget && HUDWidget->IsDialogueActive())
	{
		HUDWidget->AdvanceDialogue();
	}
}

void ARTSPlayerController::OnRightClick()
{
	CancelPlacement();
}

void ARTSPlayerController::OnCancelPlacementKey()
{
	CancelPlacement();
}

bool ARTSPlayerController::IsPointerOverInteractiveUI() const
{
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}

	const FWidgetPath Path = FSlateApplication::Get().LocateWindowUnderMouse(
		FSlateApplication::Get().GetCursorPos(),
		FSlateApplication::Get().GetInteractiveTopLevelWindows());
	if (!Path.IsValid())
	{
		return false;
	}

	for (int32 Index = Path.Widgets.Num() - 1; Index >= 0; --Index)
	{
		const TSharedRef<SWidget>& Widget = Path.Widgets[Index].Widget;
		if (!Widget->GetVisibility().IsVisible() || !Widget->IsInteractable())
		{
			continue;
		}

		const FName Type = Widget->GetType();
		const FString TypeStr = Type.ToString();
		if (TypeStr.Contains(TEXT("SViewport"))
			|| TypeStr.Contains(TEXT("SGameLayerManager"))
			|| TypeStr.Contains(TEXT("SObjectWidget"))
			|| TypeStr.Contains(TEXT("SOverlay"))
			|| TypeStr.Contains(TEXT("SCanvas"))
			|| TypeStr.Contains(TEXT("SConstraintCanvas"))
			|| TypeStr.Contains(TEXT("SVerticalBox"))
			|| TypeStr.Contains(TEXT("SHorizontalBox"))
			|| TypeStr.Contains(TEXT("SBox")))
		{
			continue;
		}

		if (TypeStr.Contains(TEXT("SButton"))
			|| TypeStr.Contains(TEXT("SCheckBox"))
			|| TypeStr.Contains(TEXT("SSlider"))
			|| TypeStr.Contains(TEXT("SEditableText")))
		{
			return true;
		}
	}
	return false;
}

void ARTSPlayerController::ClearSelectedUnit()
{
	if (SelectedUnit)
	{
		SelectedUnit->SetSelectedHighlight(false);
	}
	SelectedUnit = nullptr;
	DestroyModeRing();
}

void ARTSPlayerController::DestroyModeRing()
{
	if (ARTSUnitModeRing* Ring = ModeRing.Get())
	{
		Ring->Destroy();
	}
	ModeRing.Reset();
}

void ARTSPlayerController::ShowModeRingForSelected()
{
	if (!SelectedUnit || !SelectedUnit->IsAlive() || !SelectedUnit->IsFarmUnit() || bPlacementPending)
	{
		DestroyModeRing();
		return;
	}

	if (ARTSUnitModeRing* Existing = ModeRing.Get())
	{
		Existing->SetFollowUnit(SelectedUnit);
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ARTSUnitModeRing* Ring = GetWorld()->SpawnActor<ARTSUnitModeRing>(
		ARTSUnitModeRing::StaticClass(),
		SelectedUnit->GetActorLocation(),
		FRotator::ZeroRotator,
		Params);
	if (Ring)
	{
		Ring->SetFollowUnit(SelectedUnit);
		ModeRing = Ring;
	}
}

void ARTSPlayerController::SelectUnitUnderCursor()
{
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

	// Clicking the mode ring / its widgets should not clear selection.
	if (bHit)
	{
		if (Cast<ARTSUnitModeRing>(Hit.GetActor()))
		{
			return;
		}
	}

	ClearSelectedUnit();

	if (bHit)
	{
		SelectedUnit = Cast<ARTSUnitBase>(Hit.GetActor());
	}

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
		ShowModeRingForSelected();
	}
	else
	{
		SelectedUnit = nullptr;
	}
}

void ARTSPlayerController::HandleWorldClickForPlacement()
{
	FVector WorldOrigin;
	FVector WorldDir;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDir))
	{
		return;
	}

	const FVector TraceEnd = WorldOrigin + WorldDir * 100000.f;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RTSPlace), true);
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

	const FVector Probe = bHit ? Hit.ImpactPoint : (WorldOrigin + WorldDir * 3000.f);

	// Clicking a farm unit while placing: select it and cancel placement.
	if (bHit)
	{
		if (Cast<ARTSUnitModeRing>(Hit.GetActor()))
		{
			return;
		}
		if (ARTSUnitBase* HitUnit = Cast<ARTSUnitBase>(Hit.GetActor()))
		{
			if (HitUnit->IsAlive() && HitUnit->IsFarmUnit())
			{
				CancelPlacement();
				ClearSelectedUnit();
				SelectedUnit = HitUnit;
				SelectedUnit->SetSelectedHighlight(true);
				ShowModeRingForSelected();
				return;
			}
		}
	}

	const int32 LaneIndex = FindNearestLaneIndex(Probe);
	if (LaneIndex < 0)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(2, 2.f, FColor::Orange, TEXT("No lane near click"));
		}
		return;
	}
	ConfirmRecruitOnLane(LaneIndex);
}

int32 ARTSPlayerController::FindNearestLaneIndex(const FVector& WorldLocation) const
{
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARTSLaneSpline::StaticClass(), Found);
	if (Found.Num() == 0)
	{
		return -1;
	}

	ARTSLaneSpline* BestLane = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (AActor* Actor : Found)
	{
		ARTSLaneSpline* Lane = Cast<ARTSLaneSpline>(Actor);
		if (!Lane)
		{
			continue;
		}
		const float DistAlong = Lane->GetDistanceForWorldLocation(WorldLocation);
		const FVector OnLane = Lane->GetLocationAtDistance(DistAlong);
		// Prefer XY distance so camera height / Z doesn't dominate.
		const FVector Delta = OnLane - WorldLocation;
		const float DistSq = Delta.X * Delta.X + Delta.Y * Delta.Y;
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestLane = Lane;
		}
	}
	if (!BestLane)
	{
		return -1;
	}

	if (const ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		const int32 ArrayIndex = GM->FindLaneArrayIndex(BestLane);
		if (ArrayIndex >= 0)
		{
			return ArrayIndex;
		}
	}
	return BestLane->LaneIndex;
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
	if (bPlacementPending)
	{
		SetHighlightedLane(SelectedLaneIndex);
		ConfirmRecruitOnLane(SelectedLaneIndex);
	}
}

void ARTSPlayerController::SelectRecruitCard(ERTSUnitType Type)
{
	if (!RTSUnitData::IsFarmRecruitable(Type))
	{
		return;
	}

	ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM)
	{
		return;
	}

	// Toggle off if same card clicked again.
	if (bPlacementPending && PendingRecruitType == Type)
	{
		CancelPlacement();
		return;
	}

	const int32 Cost = GM->GetEffectiveFodderCost(Type);
	if (GM->Fodder < Cost)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(2, 2.f, FColor::Orange,
				FString::Printf(TEXT("Not enough fodder (need %d)"), Cost));
		}
		return;
	}

	PendingRecruitType = Type;
	bPlacementPending = true;
	ClearSelectedUnit();
	UpdatePlacementLaneHighlight();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2, 3.f, FColor::Cyan,
			TEXT("Card selected — click a lane to place. RMB/Esc cancel"));
	}
}

void ARTSPlayerController::ConfirmRecruitOnLane(int32 LaneIndex)
{
	if (!bPlacementPending)
	{
		SelectedLaneIndex = FMath::Max(0, LaneIndex);
		return;
	}

	SelectedLaneIndex = FMath::Max(0, LaneIndex);
	const ERTSUnitType Type = PendingRecruitType;
	ARTSUnitBase* Unit = SpawnRecruit(Type, SelectedLaneIndex);
	bPlacementPending = false;
	ClearLaneHighlights();
	if (Unit)
	{
		ClearSelectedUnit();
		SelectedUnit = Unit;
		SelectedUnit->SetSelectedHighlight(true);
		ShowModeRingForSelected();
	}
}

void ARTSPlayerController::CancelPlacement()
{
	if (!bPlacementPending)
	{
		return;
	}
	bPlacementPending = false;
	ClearLaneHighlights();
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2, 1.5f, FColor::Silver, TEXT("Placement cancelled"));
	}
}

bool ARTSPlayerController::GetMouseWorldProbe(FVector& OutProbe) const
{
	FVector WorldOrigin;
	FVector WorldDir;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDir))
	{
		return false;
	}

	const FVector TraceEnd = WorldOrigin + WorldDir * 100000.f;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RTSLaneHover), true);
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

	OutProbe = bHit ? Hit.ImpactPoint : (WorldOrigin + WorldDir * 3000.f);
	return true;
}

void ARTSPlayerController::UpdatePlacementLaneHighlight()
{
	FVector Probe;
	if (!GetMouseWorldProbe(Probe))
	{
		return;
	}
	SetHighlightedLane(FindNearestLaneIndex(Probe));
}

void ARTSPlayerController::SetHighlightedLane(int32 LaneIndex)
{
	if (HighlightedLaneIndex == LaneIndex)
	{
		// Still refresh the active lane each frame via its Tick; only skip mass clear/set.
		if (LaneIndex >= 0)
		{
			if (ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this)))
			{
				if (ARTSLaneSpline* Lane = GM->GetLaneByIndex(LaneIndex))
				{
					Lane->SetHighlighted(true);
				}
			}
		}
		return;
	}

	ClearLaneHighlights();
	HighlightedLaneIndex = LaneIndex;
	if (LaneIndex < 0)
	{
		return;
	}

	if (ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (ARTSLaneSpline* Lane = GM->GetLaneByIndex(LaneIndex))
		{
			Lane->SetHighlighted(true);
		}
	}
}

void ARTSPlayerController::ClearLaneHighlights()
{
	HighlightedLaneIndex = -1;
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARTSLaneSpline::StaticClass(), Found);
	for (AActor* Actor : Found)
	{
		if (ARTSLaneSpline* Lane = Cast<ARTSLaneSpline>(Actor))
		{
			Lane->SetHighlighted(false);
		}
	}
}

void ARTSPlayerController::RecruitSelectedType()
{
	SelectRecruitCard(PendingRecruitType);
}

ARTSUnitBase* ARTSPlayerController::SpawnRecruit(ERTSUnitType Type, int32 LaneIndex)
{
	ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
				TEXT("Recruit failed: GameMode is not RTSGameMode"));
		}
		return nullptr;
	}

	ARTSUnitBase* Unit = GM->RecruitUnit(Type, LaneIndex, EUnitWorkMode::Combat);
	if (!Unit && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2, 2.f, FColor::Orange,
			TEXT("Recruit failed (fodder / no lane?)"));
	}
	return Unit;
}

void ARTSPlayerController::RecruitType(ERTSUnitType Type)
{
	// Hotkeys: PvZ card select (then click lane). Shift-free convenience: if not placing, select card.
	SelectRecruitCard(Type);
}

void ARTSPlayerController::SetSelectedUnitWorkMode(EUnitWorkMode Mode)
{
	if (SelectedUnit && SelectedUnit->IsAlive() && SelectedUnit->IsFarmUnit())
	{
		SelectedUnit->SetWorkMode(Mode);
		if (ARTSUnitModeRing* Ring = ModeRing.Get())
		{
			Ring->RefreshModeHighlight();
		}
	}
}

void ARTSPlayerController::ToggleSelectedUnitWorkMode()
{
	if (!SelectedUnit || !SelectedUnit->IsAlive() || !SelectedUnit->IsFarmUnit())
	{
		ClearSelectedUnit();
		SelectedUnit = FindNearestFarmUnit();
		if (SelectedUnit)
		{
			SelectedUnit->SetSelectedHighlight(true);
			ShowModeRingForSelected();
		}
	}

	if (!SelectedUnit || !SelectedUnit->IsAlive())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(3, 2.f, FColor::Orange,
				TEXT("No farm unit to toggle — pick a card and place first"));
		}
		return;
	}

	const EUnitWorkMode Next = SelectedUnit->WorkMode == EUnitWorkMode::Combat
		? EUnitWorkMode::Collect
		: EUnitWorkMode::Combat;
	SetSelectedUnitWorkMode(Next);
}

void ARTSPlayerController::HotkeyRecruitRabbit() { SelectRecruitCard(ERTSUnitType::Rabbit); }
void ARTSPlayerController::HotkeyRecruitChicken() { SelectRecruitCard(ERTSUnitType::Chicken); }
void ARTSPlayerController::HotkeyRecruitSheep() { SelectRecruitCard(ERTSUnitType::Sheep); }
void ARTSPlayerController::HotkeyRecruitPig() { SelectRecruitCard(ERTSUnitType::Pig); }
void ARTSPlayerController::HotkeyLane0() { SetSelectedLane(0); }
void ARTSPlayerController::HotkeyLane1() { SetSelectedLane(1); }
void ARTSPlayerController::HotkeyToggleMode() { ToggleSelectedUnitWorkMode(); }

void ARTSPlayerController::UpgradeUnitType(ERTSUnitType Type)
{
	ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM)
	{
		return;
	}

	const int32 Before = GM->GetUnitUpgradeLevel(Type);
	if (Before >= 3)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(5, 2.f, FColor::Orange, TEXT("Upgrade failed: already max tier"));
		}
		return;
	}

	const int32 Cost = GM->GetUpgradeCost(Before + 1);
	if (GM->Soul < Cost)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(5, 2.f, FColor::Orange,
				FString::Printf(TEXT("Upgrade failed: need %d soul (have %d)"), Cost, GM->Soul));
		}
		return;
	}

	if (GM->TryUpgradeUnitType(Type))
	{
		const int32 NewLevel = GM->GetUnitUpgradeLevel(Type);
		if (HUDWidget)
		{
			HUDWidget->ShowUpgradeToast(Type, NewLevel);
		}
	}
}

void ARTSPlayerController::HotkeyUpgradeRabbit() { UpgradeUnitType(ERTSUnitType::Rabbit); }
void ARTSPlayerController::HotkeyUpgradeChicken() { UpgradeUnitType(ERTSUnitType::Chicken); }
void ARTSPlayerController::HotkeyUpgradeSheep() { UpgradeUnitType(ERTSUnitType::Sheep); }
void ARTSPlayerController::HotkeyUpgradePig() { UpgradeUnitType(ERTSUnitType::Pig); }
