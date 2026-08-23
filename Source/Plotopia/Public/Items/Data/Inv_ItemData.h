// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "Inv_ItemData.generated.h"

class UTexture2D;

/**
 * Item data row - configured in DataTable (template layer)
 */
USTRUCT(BlueprintType)
struct FInv_ItemDataRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	/** Item name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FText ItemName;

	/** Item description */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FText ItemDescription;

	/** Item icon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TSoftObjectPtr<UTexture2D> ItemIcon;

	/** Max stack size (1 = not stackable, >1 = stackable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 MaxStackSize = 1;

	/** Static mesh used when dropped into the world */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TSoftObjectPtr<UStaticMesh> WorldMesh;

	/** Whether this item can stack */
	bool CanStack() const { return MaxStackSize > 1; }
};

/**
 * Single item instance in inventory (instance layer)
 * Array maintains fixed length, empty slots use EmptySlot()
 */
USTRUCT(BlueprintType)
struct FInv_ItemInstance
{
	GENERATED_BODY()

public:
	/** Item ID (matches DataTable row name), NAME_None means empty */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FName ItemID;

	/** Item quantity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Quantity = 0;

	/** Default constructor - creates empty slot */
	FInv_ItemInstance() : ItemID(NAME_None), Quantity(0) {}

	FInv_ItemInstance(FName InItemID, int32 InQuantity = 1) : ItemID(InItemID), Quantity(InQuantity) {}

	/** Whether this slot has a valid item */
	bool IsValid() const { return ItemID != NAME_None && Quantity > 0; }

	/** Whether this slot is empty */
	bool IsEmpty() const { return !IsValid(); }

	/** Create an empty slot instance */
	static FInv_ItemInstance EmptySlot() { return FInv_ItemInstance(); }
};

/**
 * Drag and drop payload data
 * Carries complete item info in DragDropOperation
 */
USTRUCT(BlueprintType)
struct FInv_DragPayload
{
	GENERATED_BODY()

public:
	/** Source slot index */
	UPROPERTY(BlueprintReadWrite, Category = "DragDrop")
	int32 FromSlotIndex = -1;

	/** The dragged item data */
	UPROPERTY(BlueprintReadWrite, Category = "DragDrop")
	FInv_ItemInstance DraggedItem;

	/** Item icon for drag visual (optional) */
	UPROPERTY(BlueprintReadWrite, Category = "DragDrop")
	TObjectPtr<UTexture2D> DragIcon;

	FInv_DragPayload() : FromSlotIndex(-1), DragIcon(nullptr) {}
	FInv_DragPayload(int32 InFromSlot, const FInv_ItemInstance& InItem, UTexture2D* InIcon)
		: FromSlotIndex(InFromSlot), DraggedItem(InItem), DragIcon(InIcon) {}

	bool IsValid() const { return FromSlotIndex >= 0 && DraggedItem.IsValid(); }
};