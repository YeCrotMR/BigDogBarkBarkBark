// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTSBaseBuilding.generated.h"

class UStaticMeshComponent;

UCLASS()
class BIGDOGBARKBARKBARK_API ARTSBaseBuilding : public AActor
{
	GENERATED_BODY()

public:
	ARTSBaseBuilding();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Building")
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Building")
	float MaxHealth = 500.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS|Building")
	float CurrentHealth = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Building")
	bool bIsCoreBuilding = true;

	UFUNCTION(BlueprintCallable, Category = "RTS|Building")
	void ReceiveDamage(float Amount, AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category = "RTS|Building")
	bool IsAlive() const { return CurrentHealth > 0.f; }

protected:
	virtual void BeginPlay() override;
};
