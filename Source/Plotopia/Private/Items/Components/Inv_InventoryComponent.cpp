// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/Components/Inv_InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/GAS_PlayerController.h"

UInv_InventoryComponent::UInv_InventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// 组件需要参与复制（服务器权威数据源，复制给所属客户端）
	SetIsReplicatedByDefault(true);
	// Do NOT call InitializeSlots() here -- MaxSlots may not yet be set from Blueprint/CDO defaults.
	// BeginPlay() will handle initialization with the final configuration values.
}

void UInv_InventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Items);
	DOREPLIFETIME(ThisClass, MaxSlots);
	DOREPLIFETIME(ThisClass, HotbarSlotCount);
	// 物品数据表随组件复制：确保任何一端（尤其客户端）都能解析物品图标/名称，
	// 不依赖各端蓝图默认值是否一致。
	DOREPLIFETIME(ThisClass, ItemDataTable);
}

void UInv_InventoryComponent::OnRep_Items()
{
	// 服务器权威数据到达：重建派生缓存并通知UI刷新
	RebuildCache();
	BroadcastUpdate(-1);
	UE_LOG(LogTemp, Log, TEXT("[Inv] OnRep_Items (%s) 收到 %d 格数据"), *GetName(), Items.Num());
}

void UInv_InventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	// Only auto-initialize if Items is empty (not already manually initialized via InitializeInventory)
	if (Items.Num() == 0)
	{
		InitializeSlots();
	}
	BroadcastUpdate(-1);
}

void UInv_InventoryComponent::InitializeInventory(int32 InMaxSlots, int32 InHotbarCount, UDataTable* InDataTable)
{
	MaxSlots = InMaxSlots;
	HotbarSlotCount = InHotbarCount;
	ItemDataTable = InDataTable;

	InitializeSlots();
}

void UInv_InventoryComponent::InitializeSlots()
{
	Items.Empty();
	Items.SetNum(MaxSlots);
	for (int32 i = 0; i < MaxSlots; ++i)
	{
		Items[i] = FInv_ItemInstance::EmptySlot();
	}
	RebuildCache();
}

void UInv_InventoryComponent::RebuildCache()
{
	ItemCountCache.Empty();
	EmptySlotCache.Empty();
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (Items[i].IsValid())
		{
			ItemCountCache.FindOrAdd(Items[i].ItemID) += Items[i].Quantity;
		}
		else
		{
			EmptySlotCache.Add(i);
		}
	}
}

void UInv_InventoryComponent::NotifySlotChanged(int32 SlotIndex)
{
	UpdateSlotCache(SlotIndex);
}

void UInv_InventoryComponent::UpdateSlotCache(int32 SlotIndex)
{
	// For simplicity and correctness, rebuild the entire cache.
	// SlotIndex is provided for potential future incremental optimization.
	RebuildCache();
}

void UInv_InventoryComponent::BroadcastSlotsUpdated(int32 SlotA, int32 SlotB)
{
	// 数据版本号递增：供UI轮询检测变化（动态委托在跨模块场景下可能不触发，版本号是保险机制）
	InventoryVersion++;
	// Broadcast both slots individually so all listeners (backpack + hotbar) update both positions
	OnInventoryUpdated.Broadcast(Items, SlotA);
	OnInventoryUpdated.Broadcast(Items, SlotB);
	PushStateToClient();
}

/** 服务器权威端把完整背包状态通过可靠RPC推给所属客户端（客户端UI刷新的保证通道） */
void UInv_InventoryComponent::PushStateToClient()
{
	if (HasAuthority() && GetOwner() && !bApplyingServerState)
	{
		if (AGAS_PlayerController* PC = Cast<AGAS_PlayerController>(GetOwner()))
		{
			PC->Client_ReceiveInventoryState(Items, MaxSlots, HotbarSlotCount);
		}
	}
}

int32 UInv_InventoryComponent::GetItemMaxStackSize(FName ItemID) const
{
	if (ItemDataTable && ItemID != NAME_None)
	{
		static const FString ContextStr(TEXT("GetItemMaxStackSize"));
		FInv_ItemDataRow* Row = ItemDataTable->FindRow<FInv_ItemDataRow>(ItemID, ContextStr);
		if (Row)
		{
			return Row->MaxStackSize;
		}
	}
	return DefaultMaxStackSize;
}

// ==================== 服务器权威包装：客户端调用自动路由到服务器 ====================

int32 UInv_InventoryComponent::AddItem(FName ItemID, int32 Quantity)
{
	if (!HasAuthority())
	{
		Server_AddItem(ItemID, Quantity);
		return 0;
	}

	if (Quantity <= 0 || ItemID == NAME_None) return 0;

	int32 Remaining = Quantity;
	int32 ChangedSlot = -1;
	const int32 MaxPerSlot = GetItemMaxStackSize(ItemID);

	// Step 1: Try to stack onto existing items of same type
	for (int32 i = 0; i < Items.Num() && Remaining > 0; ++i)
	{
		if (Items[i].ItemID == ItemID)
		{
			int32 CanAdd = FMath::Min(Remaining, MaxPerSlot - Items[i].Quantity);
			if (CanAdd > 0)
			{
				Items[i].Quantity += CanAdd;
				Remaining -= CanAdd;
				ChangedSlot = i;
			}
		}
	}

	// Step 2: Fill remaining into empty slots (use EmptySlotCache)
	{
		// 注意：此处快照可能含 Step1 刚占用的槽位，但下面用 IsEmpty() 二次校验，
		// 因此即使缓存暂未重建也安全；整个操作的缓存只在最后统一重建一次。
		TArray<int32> EmptySlots = EmptySlotCache.Array();
		for (int32 i = 0; i < EmptySlots.Num() && Remaining > 0; ++i)
		{
			const int32 SlotIdx = EmptySlots[i];
			if (Items[SlotIdx].IsEmpty())
			{
				int32 AddNow = FMath::Min(Remaining, MaxPerSlot);
				Items[SlotIdx] = FInv_ItemInstance(ItemID, AddNow);
				Remaining -= AddNow;
				ChangedSlot = SlotIdx;
			}
		}
	}

	// 一次操作只做一次缓存重建（避免每个受影响槽位各重建一次）
	RebuildCache();
	BroadcastUpdate(ChangedSlot);
	UE_LOG(LogTemp, Log, TEXT("[Inv][Server] AddItem %s x%d 实际添加 %d"), *ItemID.ToString(), Quantity, Quantity - Remaining);
	return Quantity - Remaining;
}

void UInv_InventoryComponent::Server_AddItem_Implementation(FName ItemID, int32 Quantity)
{
	AddItem(ItemID, Quantity);
}

int32 UInv_InventoryComponent::RemoveItem(FName ItemID, int32 Quantity)
{
	if (!HasAuthority())
	{
		Server_RemoveItem(ItemID, Quantity);
		return 0;
	}

	if (Quantity <= 0 || ItemID == NAME_None) return 0;

	int32 Remaining = Quantity;
	int32 ChangedSlot = -1;
	bool bMultipleSlotsChanged = false;

	for (int32 i = 0; i < Items.Num() && Remaining > 0; ++i)
	{
		if (Items[i].ItemID == ItemID)
		{
			int32 RemoveNow = FMath::Min(Remaining, Items[i].Quantity);
			Items[i].Quantity -= RemoveNow;
			Remaining -= RemoveNow;

			if (ChangedSlot >= 0 && ChangedSlot != i)
			{
				bMultipleSlotsChanged = true;
			}
			ChangedSlot = i;

			if (Items[i].Quantity <= 0)
			{
				Items[i] = FInv_ItemInstance::EmptySlot();
			}
		}
	}

	if (Remaining < Quantity)
	{
		// 一次操作只做一次缓存重建
		RebuildCache();
		BroadcastUpdate(bMultipleSlotsChanged ? -1 : ChangedSlot);
	}
	return Quantity - Remaining;
}

void UInv_InventoryComponent::Server_RemoveItem_Implementation(FName ItemID, int32 Quantity)
{
	RemoveItem(ItemID, Quantity);
}

bool UInv_InventoryComponent::RemoveItemAtSlot(int32 SlotIndex, int32 Quantity)
{
	if (!HasAuthority())
	{
		Server_RemoveItemAtSlot(SlotIndex, Quantity);
		return false;
	}

	if (!Items.IsValidIndex(SlotIndex) || Quantity <= 0) return false;

	FInv_ItemInstance& Item = Items[SlotIndex];
	if (Item.ItemID == NAME_None || Item.Quantity < Quantity) return false;

	Item.Quantity -= Quantity;
	if (Item.Quantity <= 0)
	{
		Items[SlotIndex] = FInv_ItemInstance::EmptySlot();
	}

	NotifySlotChanged(SlotIndex);
	BroadcastUpdate(SlotIndex);
	return true;
}

void UInv_InventoryComponent::Server_RemoveItemAtSlot_Implementation(int32 SlotIndex, int32 Quantity)
{
	RemoveItemAtSlot(SlotIndex, Quantity);
}

bool UInv_InventoryComponent::HasItem(FName ItemID, int32 Quantity) const
{
	return GetItemCount(ItemID) >= Quantity;
}

int32 UInv_InventoryComponent::GetItemCount(FName ItemID) const
{
	if (const int32* Count = ItemCountCache.Find(ItemID))
	{
		return *Count;
	}
	return 0;
}

const FInv_ItemInstance& UInv_InventoryComponent::GetItemAtSlot(int32 SlotIndex) const
{
	if (Items.IsValidIndex(SlotIndex))
	{
		return Items[SlotIndex];
	}
	static FInv_ItemInstance EmptyItem;
	return EmptyItem;
}

void UInv_InventoryComponent::ClearInventory()
{
	if (!HasAuthority())
	{
		Server_ClearInventory();
		return;
	}

	for (int32 i = 0; i < Items.Num(); ++i)
	{
		Items[i] = FInv_ItemInstance::EmptySlot();
	}
	RebuildCache();
	BroadcastUpdate(-1);
}

void UInv_InventoryComponent::Server_ClearInventory_Implementation()
{
	ClearInventory();
}

int32 UInv_InventoryComponent::GetUsedSlotsCount() const
{
	return Items.Num() - EmptySlotCache.Num();
}

int32 UInv_InventoryComponent::GetAvailableSlots() const
{
	return EmptySlotCache.Num();
}

bool UInv_InventoryComponent::IsFull() const
{
	return EmptySlotCache.Num() == 0;
}

void UInv_InventoryComponent::NotifyInventoryUpdated()
{
	BroadcastUpdate(-1);
}

void UInv_InventoryComponent::SetBackpackEquipped(bool bEquipped)
{
	if (!HasAuthority())
	{
		Server_SetBackpackEquipped(bEquipped);
		return;
	}

	if (bEquipped == bIsBackpackEquipped) return;

	bIsBackpackEquipped = bEquipped;

	if (bEquipped)
	{
		Items.SetNum(MaxSlots);
	}
	else
	{
		bool bHasItemsBeyondHotbar = false;
		for (int32 i = HotbarSlotCount; i < Items.Num(); ++i)
		{
			if (Items[i].IsValid())
			{
				bHasItemsBeyondHotbar = true;
				break;
			}
		}

		if (bHasItemsBeyondHotbar)
		{
			UE_LOG(LogTemp, Warning, TEXT("SetBackpackEquipped(false) blocked: items exist beyond hotbar slots. Clear them first."));
			bIsBackpackEquipped = true;
			return;
		}

		MaxSlots = HotbarSlotCount;
		Items.SetNum(MaxSlots);
	}

	RebuildCache();
	BroadcastUpdate(-1);
}

void UInv_InventoryComponent::Server_SetBackpackEquipped_Implementation(bool bEquipped)
{
	SetBackpackEquipped(bEquipped);
}

void UInv_InventoryComponent::SwapItems(int32 SlotIndexA, int32 SlotIndexB)
{
	if (!HasAuthority())
	{
		Server_SwapItems(SlotIndexA, SlotIndexB);
		return;
	}

	if (SlotIndexA == SlotIndexB) return;
	if (!Items.IsValidIndex(SlotIndexA) || !Items.IsValidIndex(SlotIndexB)) return;

	FInv_ItemInstance Temp = Items[SlotIndexA];
	Items[SlotIndexA] = Items[SlotIndexB];
	Items[SlotIndexB] = Temp;

	// 两个槽位一次操作只重建一次缓存
	RebuildCache();
	BroadcastSlotsUpdated(SlotIndexA, SlotIndexB);
}

void UInv_InventoryComponent::Server_SwapItems_Implementation(int32 SlotIndexA, int32 SlotIndexB)
{
	SwapItems(SlotIndexA, SlotIndexB);
}

void UInv_InventoryComponent::MoveItem(int32 FromSlotIndex, int32 ToSlotIndex, int32 Amount)
{
	if (!HasAuthority())
	{
		Server_MoveItem(FromSlotIndex, ToSlotIndex, Amount);
		return;
	}

	if (!Items.IsValidIndex(FromSlotIndex) || !Items.IsValidIndex(ToSlotIndex)) return;
	if (FromSlotIndex == ToSlotIndex || Amount <= 0) return;

	FInv_ItemInstance& FromItem = Items[FromSlotIndex];
	FInv_ItemInstance& ToItem = Items[ToSlotIndex];

	if (!FromItem.IsValid()) return;
	Amount = FMath::Min(Amount, FromItem.Quantity);

	// Case 1: Target slot is empty -> move item there
	if (ToItem.IsEmpty())
	{
		ToItem = FInv_ItemInstance(FromItem.ItemID, Amount);
		FromItem.Quantity -= Amount;
		if (FromItem.Quantity <= 0)
		{
			FromItem = FInv_ItemInstance::EmptySlot();
		}
	}
	// Case 2: Same item type -> try to stack
	else if (ToItem.ItemID == FromItem.ItemID)
	{
		const int32 MaxPerSlot = GetItemMaxStackSize(FromItem.ItemID);
		int32 CanAdd = FMath::Min(Amount, MaxPerSlot - ToItem.Quantity);
		if (CanAdd > 0)
		{
			ToItem.Quantity += CanAdd;
			FromItem.Quantity -= CanAdd;
			if (FromItem.Quantity <= 0)
			{
				FromItem = FInv_ItemInstance::EmptySlot();
			}
		}
	}
	// Case 3: Different item type -> swap them (Minecraft-style)
	else
	{
		FInv_ItemInstance TempItem = ToItem;
		ToItem = FromItem;
		FromItem = TempItem;
	}

	// 两个槽位一次操作只重建一次缓存
	RebuildCache();
	BroadcastSlotsUpdated(FromSlotIndex, ToSlotIndex);
}

void UInv_InventoryComponent::Server_MoveItem_Implementation(int32 FromSlotIndex, int32 ToSlotIndex, int32 Amount)
{
	MoveItem(FromSlotIndex, ToSlotIndex, Amount);
}

void UInv_InventoryComponent::SelectHotbarSlot(int32 NewSlotIndex)
{
	if (NewSlotIndex < 0 || NewSlotIndex >= HotbarSlotCount) return;
	if (NewSlotIndex == SelectedSlotIndex) return;

	SelectedSlotIndex = NewSlotIndex;
	OnHotbarSelectionChanged.Broadcast(Items, SelectedSlotIndex);
}

void UInv_InventoryComponent::SelectNextHotbarSlot()
{
	if (HotbarSlotCount <= 0) return;
	SelectHotbarSlot((SelectedSlotIndex + 1) % HotbarSlotCount);
}

void UInv_InventoryComponent::SelectPreviousHotbarSlot()
{
	if (HotbarSlotCount <= 0) return;
	int32 NewIndex = SelectedSlotIndex - 1;
	if (NewIndex < 0) NewIndex = HotbarSlotCount - 1;
	SelectHotbarSlot(NewIndex);
}

const FInv_ItemInstance& UInv_InventoryComponent::GetHotbarSelectedItem() const
{
	if (Items.IsValidIndex(SelectedSlotIndex) && Items[SelectedSlotIndex].IsValid())
	{
		return Items[SelectedSlotIndex];
	}
	static FInv_ItemInstance EmptyItem2;
	return EmptyItem2;
}

void UInv_InventoryComponent::BroadcastUpdate(int32 ChangedSlotIndex)
{
	// 数据版本号递增：供UI轮询检测变化（动态委托在跨模块场景下可能不触发，版本号是保险机制）
	InventoryVersion++;
	UE_LOG(LogTemp, Log, TEXT("[Inv] BroadcastUpdate (%s) ChangedSlot=%d Version=%d"), *GetName(), ChangedSlotIndex, InventoryVersion);
	OnInventoryUpdated.Broadcast(Items, ChangedSlotIndex);

	// 服务器权威端：每次数据变化都通过可靠RPC把完整状态推给所属客户端。
	// 动态创建在 PlayerController 上的组件，其属性复制在客户端组件晚于服务器
	// 初始复制时建立的情况下可能收不到初始数据（表现为客户端背包一直为空），
	// 可靠RPC保证客户端一定能拿到最新背包状态。
	// bApplyingServerState 防止监听服务器主机上 RPC→Apply→Broadcast→RPC 死循环。
	PushStateToClient();
}

void UInv_InventoryComponent::ApplyReplicatedState(const TArray<FInv_ItemInstance>& InItems, int32 InMaxSlots, int32 InHotbarCount)
{
	// 重入保护：监听服务器主机也会收到自己的 Client RPC（其 HasAuthority 为真），
	// 若不保护会在 Apply→Broadcast→RPC 之间无限循环。
	if (bApplyingServerState)
	{
		return;
	}
	bApplyingServerState = true;

	if (MaxSlots != InMaxSlots || HotbarSlotCount != InHotbarCount)
	{
		MaxSlots = InMaxSlots;
		HotbarSlotCount = InHotbarCount;
	}
	if (Items.Num() != InItems.Num())
	{
		Items = InItems;
	}
	else
	{
		// 保持数组长度一致，逐槽应用，避免整数组替换干扰复制阴影
		for (int32 i = 0; i < Items.Num(); ++i)
		{
			Items[i] = InItems[i];
		}
	}
	RebuildCache();
	BroadcastUpdate(-1);

	bApplyingServerState = false;
}
