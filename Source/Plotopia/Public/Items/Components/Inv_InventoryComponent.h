// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Items/Data/Inv_ItemData.h"
#include "Inv_InventoryComponent.generated.h"

class FLifetimeProperty;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryUpdated, const TArray<FInv_ItemInstance>&, Items, int32, ChangedSlotIndex);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), Blueprintable, BlueprintType)
class PLOTOPIA_API UInv_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInv_InventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ==================== Configuration ====================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config", Replicated)
	int32 MaxSlots = 36;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config", Replicated)
	int32 HotbarSlotCount = 9;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config", Replicated)
	TObjectPtr<UDataTable> ItemDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	bool bIsBackpackEquipped = false;

	// ==================== Manual Initialization ====================
	/** Initialize inventory with given parameters (call after creating component via NewObject) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeInventory(int32 InMaxSlots, int32 InHotbarCount, UDataTable* InDataTable);

	// ==================== Item Operations ====================
	/** 客户端调用时自动路由到服务器执行（服务器权威 + 复制） */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(FName ItemID, int32 Quantity = 1);

	UFUNCTION(Server, Reliable)
	void Server_AddItem(FName ItemID, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 RemoveItem(FName ItemID, int32 Quantity = 1);

	UFUNCTION(Server, Reliable)
	void Server_RemoveItem(FName ItemID, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItemAtSlot(int32 SlotIndex, int32 Quantity = 1);

	UFUNCTION(Server, Reliable)
	void Server_RemoveItemAtSlot(int32 SlotIndex, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool HasItem(FName ItemID, int32 Quantity = 1) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetItemCount(FName ItemID) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	const TArray<FInv_ItemInstance>& GetItems() const { return Items; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	const FInv_ItemInstance& GetItemAtSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearInventory();

	UFUNCTION(Server, Reliable)
	void Server_ClearInventory();

	// ==================== Queries ====================
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemMaxStackSize(FName ItemID) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetUsedSlotsCount() const;

	/** 数据版本号（UI轮询刷新用，不依赖动态委托） */
	int32 GetInventoryVersion() const { return InventoryVersion; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetAvailableSlots() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool IsFull() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void NotifyInventoryUpdated();

	/** 客户端应用服务器推送的完整背包状态（可靠RPC通道，不依赖属性复制时序） */
	void ApplyReplicatedState(const TArray<FInv_ItemInstance>& InItems, int32 InMaxSlots, int32 InHotbarCount);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetBackpackEquipped(bool bEquipped);

	UFUNCTION(Server, Reliable)
	void Server_SetBackpackEquipped(bool bEquipped);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsBackpackEquipped() const { return bIsBackpackEquipped; }

	// ==================== Drag & Drop ====================
	UFUNCTION(BlueprintCallable, Category = "Inventory|DragDrop")
	void SwapItems(int32 SlotIndexA, int32 SlotIndexB);

	UFUNCTION(Server, Reliable)
	void Server_SwapItems(int32 SlotIndexA, int32 SlotIndexB);

	UFUNCTION(BlueprintCallable, Category = "Inventory|DragDrop")
	void MoveItem(int32 FromSlotIndex, int32 ToSlotIndex, int32 Amount = 1);

	UFUNCTION(Server, Reliable)
	void Server_MoveItem(int32 FromSlotIndex, int32 ToSlotIndex, int32 Amount);

	// ==================== Hotbar ====================
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Hotbar")
	int32 SelectedSlotIndex = 0;

public:
	UFUNCTION(BlueprintPure, Category = "Inventory|Hotbar")
	int32 GetSelectedHotbarSlot() const { return SelectedSlotIndex; }

	UFUNCTION(BlueprintCallable, Category = "Inventory|Hotbar")
	void SelectHotbarSlot(int32 NewSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Hotbar")
	void SelectNextHotbarSlot();

	UFUNCTION(BlueprintCallable, Category = "Inventory|Hotbar")
	void SelectPreviousHotbarSlot();

	UFUNCTION(BlueprintPure, Category = "Inventory|Hotbar")
	const FInv_ItemInstance& GetHotbarSelectedItem() const;

	// ==================== Events ====================
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryUpdated OnInventoryUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryUpdated OnHotbarSelectionChanged;

protected:
	virtual void BeginPlay() override;

	/** 物品数据（服务器权威，整体复制到所属客户端） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", ReplicatedUsing = OnRep_Items)
	TArray<FInv_ItemInstance> Items;

	UFUNCTION()
	void OnRep_Items();

	static constexpr int32 DefaultMaxStackSize = 99;

	/** 本组件是否为服务器权威端（客户端调用会先路由到服务器） */
	bool HasAuthority() const { return GetOwner() && GetOwner()->HasAuthority(); }

	/** 数据版本号：每次数据变化递增，UI轮询检测（不依赖动态委托，跨模块安全） */
	UPROPERTY(Transient)
	int32 InventoryVersion = 0;

	/** 正在应用服务器推送的状态（防止监听服务器主机上 RPC→Apply→Broadcast→RPC 死循环） */
	bool bApplyingServerState = false;

	/** Broadcast with single slot index for targeted UI refresh */
	void BroadcastUpdate(int32 ChangedSlotIndex = -1);

	/** Broadcast with optional specific slots for targeted refresh */
	void BroadcastSlotsUpdated(int32 SlotA, int32 SlotB);

	/** 服务器权威端：把完整背包状态通过可靠RPC推给所属客户端（客户端UI刷新保证通道） */
	void PushStateToClient();

	void InitializeSlots();

	// ---------- Caching for performance ----------
	UPROPERTY(Transient)
	TMap<FName, int32> ItemCountCache;

	UPROPERTY(Transient)
	TSet<int32> EmptySlotCache;

	/** Full cache rebuild (used on initial load / bulk operations) */
	void RebuildCache();

	/** 槽位缓存更新（当前实现为全量重建，36格开销可忽略；保留槽位参数便于未来增量优化） */
	void UpdateSlotCache(int32 SlotIndex);

	/** Notify slot changed + incrementally update cache */
	void NotifySlotChanged(int32 SlotIndex);
};
