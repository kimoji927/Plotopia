// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/Components/Inv_InventoryComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/GAS_BaseCharacter.h"
#include "GameplayEffect.h"
#include "GameplayTags/GASTags.h"
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

	// 数据表可能已更换：物品行缓存必须失效
	ItemDataRowCache.Empty();

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

// ==================== 消耗品使用（右键使用 / GAS） ====================

bool UInv_InventoryComponent::GetCachedItemDataRow(FName ItemID, FInv_ItemDataRow& OutRow)
{
	if (ItemID == NAME_None) return false;

	// 命中缓存：DataTable 查找 + 结构体拷贝（含TArray/TSoftObjectPtr）对每帧UI刷新来说太贵
	if (const FInv_ItemDataRow* Cached = ItemDataRowCache.Find(ItemID))
	{
		OutRow = *Cached;
		return true;
	}

	if (!ItemDataTable) return false;

	static const FString ContextStr(TEXT("Inv_InventoryComponent::GetItemDataRow"));
	if (const FInv_ItemDataRow* Row = ItemDataTable->FindRow<FInv_ItemDataRow>(ItemID, ContextStr))
	{
		OutRow = *Row;
		ItemDataRowCache.Add(ItemID, OutRow);
		return true;
	}
	return false;
}

bool UInv_InventoryComponent::GetItemDataRow(FName ItemID, FInv_ItemDataRow& OutRow)
{
	return GetCachedItemDataRow(ItemID, OutRow);
}

bool UInv_InventoryComponent::IsConsumableItem(FName ItemID)
{
	FInv_ItemDataRow Row;
	return GetCachedItemDataRow(ItemID, Row) && Row.IsConsumable();
}

UAbilitySystemComponent* UInv_InventoryComponent::GetOwnerAbilitySystemComponent() const
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner)) return nullptr;

	// 本组件挂在玩家控制器上，而玩家ASC实际挂在 Pawn / PlayerState 上，这里统一解析出ASC
	if (const APlayerController* PC = Cast<APlayerController>(Owner))
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			if (UAbilitySystemComponent* PawnASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
			{
				return PawnASC;
			}
		}
	}
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
}

bool UInv_InventoryComponent::IsOwnerAlive() const
{
	if (const APlayerController* PC = Cast<APlayerController>(GetOwner()))
	{
		if (const AGAS_BaseCharacter* BaseCharacter = Cast<AGAS_BaseCharacter>(PC->GetPawn()))
		{
			return BaseCharacter->IsAlive();
		}
	}
	// 宿主不是玩家控制器/不是GAS角色时不做存活限制
	return true;
}

bool UInv_InventoryComponent::CanUseItemAtSlot(int32 SlotIndex, FText& OutFailReason)
{
	OutFailReason = FText::GetEmpty();

	if (!Items.IsValidIndex(SlotIndex))
	{
		OutFailReason = NSLOCTEXT("Inventory", "UseItem_BadSlot", "槽位无效");
		return false;
	}

	const FInv_ItemInstance& Item = Items[SlotIndex];
	if (!Item.IsValid())
	{
		OutFailReason = NSLOCTEXT("Inventory", "UseItem_EmptySlot", "该槽位没有物品");
		return false;
	}

	FInv_ItemDataRow Row;
	if (!GetCachedItemDataRow(Item.ItemID, Row))
	{
		OutFailReason = NSLOCTEXT("Inventory", "UseItem_NoRow", "找不到该物品的数据行（DT_Items）");
		return false;
	}

	if (!Row.IsConsumable())
	{
		OutFailReason = NSLOCTEXT("Inventory", "UseItem_NotConsumable", "该物品不是消耗品");
		return false;
	}

	if (Row.bConsumeOnUse && Item.Quantity < FMath::Max(1, Row.ConsumeCount))
	{
		OutFailReason = NSLOCTEXT("Inventory", "UseItem_NotEnough", "数量不足");
		return false;
	}

	if (bBlockUseWhenDead && !IsOwnerAlive())
	{
		OutFailReason = NSLOCTEXT("Inventory", "UseItem_Dead", "已死亡，无法使用物品");
		return false;
	}

	if (!GetOwnerAbilitySystemComponent())
	{
		OutFailReason = NSLOCTEXT("Inventory", "UseItem_NoASC", "能力系统（ASC）未初始化");
		return false;
	}

	return true;
}

int32 UInv_InventoryComponent::ApplyConsumeEffects(UAbilitySystemComponent* ASC, const FInv_ItemDataRow& Row)
{
	if (!IsValid(ASC)) return 0;

	// 效果来源：物品行优先；行内为空时使用组件上的兜底效果（方便全局统一配置）
	// → 想换成别的效果（回蓝/加速/加护盾…）只需要改这里的GE，不需要改C++代码
	TArray<TSubclassOf<UGameplayEffect>> Effects;
	for (const TSubclassOf<UGameplayEffect>& EffectClass : Row.ConsumeEffects)
	{
		if (EffectClass) Effects.Add(EffectClass);
	}
	if (Effects.Num() == 0 && DefaultConsumeEffect)
	{
		Effects.Add(DefaultConsumeEffect);
	}
	if (Effects.Num() == 0) return 0;

	// SetByCaller 数值/标签：物品行 → 组件兜底 → 原生标签
	const float Magnitude = Row.ConsumeMagnitude > 0.f ? Row.ConsumeMagnitude : DefaultConsumeMagnitude;
	FGameplayTag MagnitudeTag = Row.ConsumeMagnitudeTag;
	if (!MagnitudeTag.IsValid()) MagnitudeTag = DefaultConsumeMagnitudeTag;
	if (!MagnitudeTag.IsValid()) MagnitudeTag = GASTags::SetByCaller::Consume;

	const float Level = Row.ConsumeEffectLevel > 0.f ? Row.ConsumeEffectLevel : 1.f;

	AActor* AvatarActor = ASC->GetAvatarActor();
	AActor* Causer = IsValid(AvatarActor) ? AvatarActor : GetOwner();
	AActor* InstigatorActor = IsValid(GetOwner()) ? GetOwner() : Causer;

	int32 AppliedCount = 0;
	for (const TSubclassOf<UGameplayEffect>& EffectClass : Effects)
	{
		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
		ContextHandle.AddInstigator(InstigatorActor, Causer);
		ContextHandle.AddSourceObject(this);

		const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, Level, ContextHandle);
		if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid()) continue;

		// 数值接口：GE 里用 “Set by Caller” 修饰符 + MagnitudeTag 即可读取（回血量自由调）
		if (Magnitude > 0.f && MagnitudeTag.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(MagnitudeTag, Magnitude);
		}

		// 只有真正生效的效果才计数（被 BlockedTags/免疫等挡下的GE返回未成功应用）
		const FActiveGameplayEffectHandle AppliedHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		if (AppliedHandle.WasSuccessfullyApplied())
		{
			++AppliedCount;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Inv][Consume] GameplayEffect %s 未能生效（可能被BlockedTags/免疫挡住）"), *EffectClass->GetName());
		}
	}
	return AppliedCount;
}

bool UInv_InventoryComponent::PerformUseItemAtSlot(int32 SlotIndex, FText& OutFailReason)
{
	if (!HasAuthority())
	{
		OutFailReason = NSLOCTEXT("Inventory", "UseItem_NotAuthority", "使用物品只能在服务器执行");
		return false;
	}

	if (!CanUseItemAtSlot(SlotIndex, OutFailReason)) return false;

	UAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		OutFailReason = NSLOCTEXT("Inventory", "UseItem_NoASC", "能力系统（ASC）未初始化");
		return false;
	}

	// 物品ID先拷出来：下面广播/扣除都可能改动 Items 数组
	const FName ItemID = Items[SlotIndex].ItemID;

	FInv_ItemDataRow Row;
	if (!GetCachedItemDataRow(ItemID, Row))
	{
		OutFailReason = NSLOCTEXT("Inventory", "UseItem_NoRow", "找不到该物品的数据行（DT_Items）");
		return false;
	}

	// 1) GAS核心：应用物品行里配置的GameplayEffect（回血 / 回蓝 / 加Buff…完全数据驱动）
	const int32 AppliedCount = ApplyConsumeEffects(ASC, Row);

	// 2) 可选表现：GameplayCue（必须以 GameplayCue. 开头才会生效）
	const FGameplayTag CueTag = Row.ConsumeCueTag;
	const bool bHasCue = CueTag.IsValid() && CueTag.ToString().StartsWith(TEXT("GameplayCue."));
	if (bHasCue)
	{
		FGameplayCueParameters CueParams;
		CueParams.Instigator = IsValid(ASC->GetAvatarActor()) ? ASC->GetAvatarActor() : GetOwner();
		CueParams.EffectCauser = GetOwner();
		CueParams.SourceObject = this;
		ASC->ExecuteGameplayCue(CueTag, CueParams);
	}

	// 3) 可选事件：把“使用了某物品”广播给GAS能力层（蓝图能力用 WaitGameplayEvent 监听该标签做动画/音效）
	const FGameplayTag EventTag = Row.ConsumeEventTag;
	if (EventTag.IsValid())
	{
		FGameplayEventData Payload;
		Payload.EventTag = EventTag;
		Payload.Instigator = GetOwner();
		Payload.Target = ASC->GetAvatarActor();
		Payload.OptionalObject = this;
		Payload.EventMagnitude = static_cast<float>(SlotIndex);
		ASC->HandleGameplayEvent(EventTag, &Payload);
	}

	// 既没有效果、也没有事件/Cue：视为配置缺失/被挡住，直接失败（避免白白吃掉一个物品）
	if (AppliedCount == 0 && !bHasCue && !EventTag.IsValid())
	{
		OutFailReason = NSLOCTEXT("Inventory", "UseItem_NoEffect", "该消耗品未配置使用效果，或效果全部未能生效（检查 ConsumeEffects / ConsumeCueTag / ConsumeEventTag）");
		return false;
	}

	// 4) 扣除物品（可在数据行里关掉，做成可重复使用的道具）
	if (Row.bConsumeOnUse && Items.IsValidIndex(SlotIndex))
	{
		const int32 ConsumeAmount = FMath::Max(1, Row.ConsumeCount);
		FInv_ItemInstance& Item = Items[SlotIndex];
		Item.Quantity -= ConsumeAmount;
		if (Item.Quantity <= 0)
		{
			Item = FInv_ItemInstance::EmptySlot();
		}

		RebuildCache();
		BroadcastUpdate(SlotIndex);
	}

	OnItemUsed.Broadcast(ItemID, SlotIndex, true);
	OnItemUsedBP(ItemID, SlotIndex);

	UE_LOG(LogTemp, Log, TEXT("[Inv][Consume] %s 使用槽位 %d 的物品 %s（应用效果 %d 个）"),
		*GetName(), SlotIndex, *ItemID.ToString(), AppliedCount);
	return true;
}

bool UInv_InventoryComponent::UseItemAtSlot(int32 SlotIndex)
{
	if (!HasAuthority())
	{
		// 客户端：只做最基本的槽位校验，权威判定交给服务器
		if (!Items.IsValidIndex(SlotIndex) || !Items[SlotIndex].IsValid())
		{
			OnItemUsed.Broadcast(NAME_None, SlotIndex, false);
			return false;
		}
		Server_UseItemAtSlot(SlotIndex);
		return true;
	}

	FText FailReason;
	const bool bSuccess = PerformUseItemAtSlot(SlotIndex, FailReason);
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Inv][Consume] 使用槽位 %d 失败：%s"), SlotIndex, *FailReason.ToString());
		OnItemUsed.Broadcast(Items.IsValidIndex(SlotIndex) ? Items[SlotIndex].ItemID : NAME_None, SlotIndex, false);
	}
	return bSuccess;
}

void UInv_InventoryComponent::Server_UseItemAtSlot_Implementation(int32 SlotIndex)
{
	UseItemAtSlot(SlotIndex);
}

bool UInv_InventoryComponent::UseSelectedHotbarItem()
{
	return UseItemAtSlot(SelectedSlotIndex);
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
