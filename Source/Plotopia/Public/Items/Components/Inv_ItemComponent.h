// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inv_ItemComponent.generated.h"

class FLifetimeProperty;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent),Blueprintable)
class PLOTOPIA_API UInv_ItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInv_ItemComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Item ID (matches the DataTable row name) */
	UPROPERTY(EditAnywhere, Category = "Inventory", Replicated)
	FName ItemID;

	/** Quantity of this item in the world (stack size when dropped) */
	UPROPERTY(EditAnywhere, Category = "Inventory", Replicated)
	int32 Quantity = 1;

	FString GetPickupMessage() const { return PickupMessage; }

private:
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FString PickupMessage;
};
