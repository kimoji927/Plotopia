// Inv_HotbarWidget.cpp
#include "UI/HUD/Inv_HotbarWidget.h"
#include "Items/Components/Inv_InventoryComponent.h"
#include "Player/GAS_PlayerController.h"
#include "Components/HorizontalBox.h"
#include "Engine/Engine.h"

void UInv_HotbarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// WBP_Hotbar 在蓝图的 Construct 事件里创建 9 个快捷栏槽位控件，因此必须
	// 在控件真正进入视口（Construct 之后）再绑定事件并刷新，否则槽位数组尚未
	// 创建就调用 ClearSlotDisplay/UpdateSlotDisplay 会产生“索引越界”。
	if (!InventoryComponent)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			if (AGAS_PlayerController* GASPC = Cast<AGAS_PlayerController>(PC))
			{
				InitHotbar(GASPC->GetInventoryComponent());
			}
		}
	}
	else
	{
		if (!InventoryComponent->OnInventoryUpdated.Contains(this, TEXT("OnInventorySlotUpdated")))
		{
			InventoryComponent->OnInventoryUpdated.AddDynamic(this, &UInv_HotbarWidget::OnInventorySlotUpdated);
		}
		if (!InventoryComponent->OnHotbarSelectionChanged.Contains(this, TEXT("OnSelectionChanged")))
		{
			InventoryComponent->OnHotbarSelectionChanged.AddDynamic(this, &UInv_HotbarWidget::OnSelectionChanged);
		}
		RefreshAllSlots();
		LastSeenVersion = InventoryComponent->GetInventoryVersion();
		PreviousSelectedSlot = InventoryComponent->GetSelectedHotbarSlot();
		HighlightSelectedSlot(PreviousSelectedSlot);
	}
}

void UInv_HotbarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	//版本号轮询刷新：不依赖动态委托，确保复制数据一定能刷新到快捷栏
	if (IsValid(InventoryComponent) && InventoryComponent->GetInventoryVersion() != LastSeenVersion)
	{
		LastSeenVersion = InventoryComponent->GetInventoryVersion();
		RefreshAllSlots();
		if (PreviousSelectedSlot >= 0)
		{
			HighlightSelectedSlot(PreviousSelectedSlot);
		}
	}
}

bool UInv_HotbarWidget::GetCachedItemDataRow(FName ItemID, FInv_ItemDataRow& OutRow)
{
	if (FInv_ItemDataRow* Cached = ItemDataRowCache.Find(ItemID))
	{
		OutRow = *Cached;
		return true;
	}

	if (GetItemDataRow(ItemID, OutRow))
	{
		ItemDataRowCache.Add(ItemID, OutRow);
		return true;
	}

	return false;
}

UTexture2D* UInv_HotbarWidget::GetItemIcon(FName ItemID)
{
	FInv_ItemDataRow DataRow;
	if (GetCachedItemDataRow(ItemID, DataRow))
	{
		return DataRow.ItemIcon.IsNull() ? nullptr : DataRow.ItemIcon.LoadSynchronous();
	}
	return nullptr;
}

void UInv_HotbarWidget::NativeDestruct()
{
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->OnInventoryUpdated.RemoveAll(this);
		InventoryComponent->OnHotbarSelectionChanged.RemoveAll(this);
		InventoryComponent = nullptr;
	}
	ItemDataRowCache.Empty();
	Super::NativeDestruct();
}

void UInv_HotbarWidget::InitHotbar(UInv_InventoryComponent* InInventoryComponent)
{
	InventoryComponent = InInventoryComponent;

	if (IsValid(InventoryComponent))
	{
		// Use AddUniqueDynamic to avoid duplicate bindings
		if (!InventoryComponent->OnInventoryUpdated.Contains(this, TEXT("OnInventorySlotUpdated")))
		{
			InventoryComponent->OnInventoryUpdated.AddDynamic(this, &UInv_HotbarWidget::OnInventorySlotUpdated);
		}
		if (!InventoryComponent->OnHotbarSelectionChanged.Contains(this, TEXT("OnSelectionChanged")))
		{
			InventoryComponent->OnHotbarSelectionChanged.AddDynamic(this, &UInv_HotbarWidget::OnSelectionChanged);
		}

		// 注意：这里不再立即 RefreshAllSlots —— 首次刷新由 NativeConstruct
		// （视口加入之后、蓝图槽位创建完成之后）或 NativeTick 版本轮询触发，
		// 避免在 WBP_Hotbar 蓝图尚未创建槽位数组时刷新导致“索引越界”。
		PreviousSelectedSlot = InventoryComponent->GetSelectedHotbarSlot();
		HighlightSelectedSlot(PreviousSelectedSlot);
		LastSeenVersion = InventoryComponent->GetInventoryVersion();
	}
}

void UInv_HotbarWidget::RefreshAllSlots()
{
	if (!IsValid(InventoryComponent)) return;
	for (int32 i = 0; i < InventoryComponent->HotbarSlotCount; ++i)
	{
		RefreshSlot(i);
	}
}

void UInv_HotbarWidget::RefreshSlot(int32 SlotIndex)
{
	if (!IsValid(InventoryComponent)) return;

	if (SlotIndex < 0 || SlotIndex >= InventoryComponent->HotbarSlotCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("RefreshSlot: SlotIndex %d out of bounds (0-%d)"), SlotIndex, InventoryComponent->HotbarSlotCount - 1);
		return;
	}

	const TArray<FInv_ItemInstance>& Items = InventoryComponent->GetItems();
	if (SlotIndex < Items.Num() && Items[SlotIndex].IsValid())
	{
		FInv_ItemDataRow DataRow;
		if (GetCachedItemDataRow(Items[SlotIndex].ItemID, DataRow))
		{
			UTexture2D* Icon = DataRow.ItemIcon.IsNull() ? nullptr : DataRow.ItemIcon.LoadSynchronous();
			//保底图标：DT_Items未配置ItemIcon时用引擎默认贴图，保证物品可见
			if (!Icon && GEngine)
			{
				Icon = GEngine->DefaultTexture;
			}
			UpdateSlotDisplay(SlotIndex, Icon, DataRow.ItemName, Items[SlotIndex].Quantity);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Inv][UI] Hotbar 找不到物品数据行 %s (ItemDataTable=%s)"),
				*Items[SlotIndex].ItemID.ToString(),
				InventoryComponent->ItemDataTable ? *InventoryComponent->ItemDataTable->GetName() : TEXT("NULL"));
			UpdateSlotDisplay(SlotIndex, nullptr, FText::FromName(Items[SlotIndex].ItemID), Items[SlotIndex].Quantity);
		}
	}
	else
	{
		ClearSlotDisplay(SlotIndex);
	}
}

void UInv_HotbarWidget::OnInventorySlotUpdated(const TArray<FInv_ItemInstance>& Items, int32 SlotIndex)
{
	if (IsValid(InventoryComponent) && SlotIndex >= 0 && SlotIndex < InventoryComponent->HotbarSlotCount)
	{
		RefreshSlot(SlotIndex);
	}
	else if (SlotIndex == -1)
	{
		RefreshAllSlots();
	}
}

void UInv_HotbarWidget::OnSelectionChanged(const TArray<FInv_ItemInstance>& Items, int32 SlotIndex)
{
	if (PreviousSelectedSlot >= 0 && PreviousSelectedSlot != SlotIndex)
	{
		RefreshSlot(PreviousSelectedSlot);
	}
	PreviousSelectedSlot = SlotIndex;
	HighlightSelectedSlot(SlotIndex);
}

bool UInv_HotbarWidget::GetItemDataRow_Implementation(FName ItemID, FInv_ItemDataRow& OutRow)
{
	if (!InventoryComponent) return false;
	UDataTable* DT = InventoryComponent->ItemDataTable;
	if (!DT || ItemID == NAME_None) return false;

	static const FString ContextStr(TEXT("GetItemDataRow_Hotbar"));
	if (FInv_ItemDataRow* Row = DT->FindRow<FInv_ItemDataRow>(ItemID, ContextStr))
	{
		OutRow = *Row;
		return true;
	}
	return false;
}
