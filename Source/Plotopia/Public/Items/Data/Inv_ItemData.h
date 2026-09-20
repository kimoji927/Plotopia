// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "Inv_ItemData.generated.h"

class UTexture2D;
class UGameplayEffect;

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

	// ==================== 消耗品 / GAS 使用配置 ====================

	/** 是否为消耗品：勾选后，在该物品所在槽位“右键”即可直接使用（未勾选则右键仍走原来的“数量丢弃”流程） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Consumable")
	bool bIsConsumable = false;

	/** 使用后应用的GameplayEffect（可自由更换/叠加，例如 GE_AddHealth 回血、GE_AddMana 回蓝；留空则用背包组件上的 DefaultConsumeEffect 兜底） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Consumable", meta = (EditCondition = "bIsConsumable"))
	TArray<TSubclassOf<UGameplayEffect>> ConsumeEffects;

	/** 应用效果时使用的等级（GE 内可用等级缩放/CurveTable 数值） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Consumable", meta = (EditCondition = "bIsConsumable", ClampMin = "1.0"))
	float ConsumeEffectLevel = 1.f;

	/** SetByCaller 数值：> 0 时写入下面的标签，供 GE 里的“Set by Caller”修饰符读取（回血量/回蓝量等自由调） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Consumable", meta = (EditCondition = "bIsConsumable", ClampMin = "0.0"))
	float ConsumeMagnitude = 0.f;

	/** SetByCaller 标签；留空则使用背包组件上的 DefaultConsumeMagnitudeTag，再留空则回落到 GASTags.SetByCaller.Consume */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Consumable", meta = (EditCondition = "bIsConsumable"))
	FGameplayTag ConsumeMagnitudeTag;

	/** 使用后是否扣除物品（取消勾选 = 无限次使用，例如可重复使用的道具） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Consumable", meta = (EditCondition = "bIsConsumable"))
	bool bConsumeOnUse = true;

	/** 每次使用扣除的数量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Consumable", meta = (EditCondition = "bIsConsumable && bConsumeOnUse", ClampMin = "1"))
	int32 ConsumeCount = 1;

	/** 使用成功时向玩家发送的 GameplayEvent（可选；蓝图能力用 WaitGameplayEvent 监听该标签做动画/音效等表现） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Consumable", meta = (EditCondition = "bIsConsumable"))
	FGameplayTag ConsumeEventTag;

	/** 使用成功时执行的 GameplayCue（可选；必须以 GameplayCue. 开头才会生效） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Consumable", meta = (EditCondition = "bIsConsumable"))
	FGameplayTag ConsumeCueTag;

	/** 该物品是否可被右键使用（C++ 辅助函数；蓝图可直接读 bIsConsumable 布尔值） */
	bool IsConsumable() const { return bIsConsumable; }
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