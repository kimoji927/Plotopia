// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Inventory/Inv_InventoryWidget.h"
#include "Items/Components/Inv_InventoryComponent.h"
#include "UI/Inventory/Inv_ItemSlotWidget.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Player/GAS_PlayerController.h"
#include "Engine/Engine.h"

UTexture2D* UInv_InventoryWidget::GetItemIcon(FName ItemID)
{
	// 图标缓存：同一物品ID只加载一次软引用，避免每次刷新重复 LoadSynchronous
	if (TObjectPtr<UTexture2D>* Cached = ItemIconCache.Find(ItemID))
	{
		return Cached->Get();
	}

	FInv_ItemDataRow ItemDataRow;
	UTexture2D* Icon = nullptr;
	if (GetCachedItemDataRow(ItemID, ItemDataRow))
	{
		Icon = ItemDataRow.ItemIcon.IsNull() ? nullptr : ItemDataRow.ItemIcon.LoadSynchronous();
		//保底图标：DT_Items未配置ItemIcon时用引擎默认贴图，保证物品可见
		if (!Icon && GEngine)
		{
			Icon = GEngine->DefaultTexture;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Inv][UI] GetItemIcon %s -> 行查找失败"), *ItemID.ToString());
	}
	ItemIconCache.Add(ItemID, Icon);
	UE_LOG(LogTemp, Log, TEXT("[Inv][UI] GetItemIcon %s -> %s (软引用空=%s)"),
		*ItemID.ToString(),
		Icon ? *Icon->GetName() : TEXT("NULL"),
		ItemDataRow.ItemIcon.IsNull() ? TEXT("是") : TEXT("否"));
	return Icon;
}

bool UInv_InventoryWidget::GetItemDataRow_Implementation(FName ItemID, FInv_ItemDataRow& OutRow)
{
	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Inv][UI] GetItemDataRow: InventoryComponent为空!"));
		return false;
	}
	UDataTable* DT = InventoryComponent->ItemDataTable;
	if (!DT || ItemID == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Inv][UI] GetItemDataRow: DataTable=%s ItemID=%s"),
			DT ? *DT->GetName() : TEXT("NULL"), *ItemID.ToString());
		return false;
	}

	static const FString ContextStr(TEXT("GetItemDataRow"));
	if (FInv_ItemDataRow* Row = DT->FindRow<FInv_ItemDataRow>(ItemID, ContextStr))
	{
		OutRow = *Row;
		return true;
	}
	UE_LOG(LogTemp, Warning, TEXT("[Inv][UI] GetItemDataRow: 找不到行 %s"), *ItemID.ToString());
	return false;
}

bool UInv_InventoryWidget::GetCachedItemDataRow(FName ItemID, FInv_ItemDataRow& OutRow)
{
	if (FInv_ItemDataRow* Cached = ItemDataRowCache.Find(ItemID))
	{
		OutRow = *Cached;
		return true;
	}

	if (GetItemDataRow_Implementation(ItemID, OutRow))
	{
		ItemDataRowCache.Add(ItemID, OutRow);
		return true;
	}

	return false;
}

void UInv_InventoryWidget::ClearItemDataRowCache()
{
	ItemDataRowCache.Empty();
	// 数据行缓存失效时图标缓存一并清空（重建格子时调用）
	ItemIconCache.Empty();
}

UInv_ItemSlotWidget* UInv_InventoryWidget::CreateSlotWidget(int32 SlotIndex, UGridPanel* TargetGrid, int32 Row, int32 Column)
{
	if (!SlotWidgetClass) return nullptr;

	UInv_ItemSlotWidget* SlotWidget = CreateWidget<UInv_ItemSlotWidget>(this, SlotWidgetClass);
	if (!SlotWidget) return nullptr;

	SlotWidget->SetParentInventory(this);
	SlotWidget->SetOnSlotSwapCallback(FOnSlotDragDropSwap::CreateUObject(
		this, &UInv_InventoryWidget::HandleSlotDragDropSwap));

	// Only set item data; icon/display will be applied by subsequent RefreshSlot/RefreshSlotWidget
	// This avoids duplicating icon loading (already done in RefreshSlotWidget)
	const TArray<FInv_ItemInstance>& Items = InventoryComponent->GetItems();
	const bool bHasItem = Items.IsValidIndex(SlotIndex) && Items[SlotIndex].IsValid();
	if (bHasItem)
	{
		SlotWidget->SetItemData(Items[SlotIndex], SlotIndex);
	}
	else
	{
		SlotWidget->SetItemData(FInv_ItemInstance::EmptySlot(), SlotIndex);
	}

	if (TargetGrid)
	{
		UGridSlot* GridSlot = TargetGrid->AddChildToGrid(SlotWidget);
		if (GridSlot)
		{
			GridSlot->SetRow(Row);
			GridSlot->SetColumn(Column);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Inv][UI] CreateSlotWidget: TargetGrid为空! 格子 %d 未加入布局（检查WBP_Inventory的InventoryGrid/HotbarGrid绑定）"), SlotIndex);
	}

	return SlotWidget;
}

void UInv_InventoryWidget::InitInventory(UInv_InventoryComponent* InInventoryComponent)
{
	if (!InInventoryComponent) return;

	InventoryComponent = InInventoryComponent;
	MaxSlots = InventoryComponent->MaxSlots;
	HotbarSlots = InventoryComponent->HotbarSlotCount;

	// Use AddUniqueDynamic to avoid duplicate bindings if InitInventory is called multiple times
	if (!InventoryComponent->OnInventoryUpdated.Contains(this, TEXT("OnInventoryUpdated")))
	{
		InventoryComponent->OnInventoryUpdated.AddDynamic(this, &UInv_InventoryWidget::OnInventoryUpdated);
	}

	UE_LOG(LogTemp, Log, TEXT("[Inv][UI] InitInventory: MaxSlots=%d Hotbar=%d SlotWidgetClass=%s ItemDataTable=%s"),
		MaxSlots, HotbarSlots,
		SlotWidgetClass ? *SlotWidgetClass->GetName() : TEXT("NULL"),
		InventoryComponent->ItemDataTable ? *InventoryComponent->ItemDataTable->GetName() : TEXT("NULL"));

	RebuildInventoryGrid();
	// 与版本号轮询同步，避免初始化后再触发一次全量刷新
	LastSeenVersion = InventoryComponent->GetInventoryVersion();
}

void UInv_InventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 控件每次（重新）进入视口时执行：确保绑定到当前玩家的背包组件并刷新。
	// 关卡旅行 / 组件重建后，之前的绑定可能已经失效（NativeDestruct 解绑），
	// 若不在此重新绑定，UI 将永远收不到 OnInventoryUpdated / 版本号变化。
	if (!InventoryComponent)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			if (AGAS_PlayerController* GASPC = Cast<AGAS_PlayerController>(PC))
			{
				InitInventory(GASPC->GetInventoryComponent());
			}
		}
	}
	else if (!InventoryComponent->OnInventoryUpdated.Contains(this, TEXT("OnInventoryUpdated")))
	{
		InventoryComponent->OnInventoryUpdated.AddDynamic(this, &UInv_InventoryWidget::OnInventoryUpdated);
		RefreshAllSlots();
		LastSeenVersion = InventoryComponent->GetInventoryVersion();
		UpdateCapacityText();
	}
}

void UInv_InventoryWidget::NativeDestruct()
{
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->OnInventoryUpdated.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UInv_InventoryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	//版本号轮询刷新：不依赖动态委托，确保服务器/复制数据一定能刷新到UI
	if (InventoryComponent && InventoryComponent->GetInventoryVersion() != LastSeenVersion)
	{
		LastSeenVersion = InventoryComponent->GetInventoryVersion();
		RefreshAllSlots();
		UpdateCapacityText();
	}
}

void UInv_InventoryWidget::ToggleInventory()
{
	if (bIsVisible) HideInventory();
	else ShowInventory();
}

void UInv_InventoryWidget::ShowInventory()
{
	if (bIsVisible) return;  // Already visible, nothing to do

	bIsVisible = true;
	SetVisibility(ESlateVisibility::Visible);
	RefreshAllSlots();
	UpdateCapacityText();
}

void UInv_InventoryWidget::HideInventory()
{
	bIsVisible = false;
	ClearSelection();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UInv_InventoryWidget::RebuildInventoryGrid()
{
	if (!InventoryComponent || !SlotWidgetClass) return;

	//诊断：确认BindWidget的网格面板和槽位类是否有效
	UE_LOG(LogTemp, Log, TEXT("[Inv][UI] RebuildGrid: InventoryGrid=%s HotbarGrid=%s MaxSlots=%d HotbarSlots=%d SlotWidgetClass=%s"),
		InventoryGrid ? TEXT("OK") : TEXT("NULL"),
		HotbarGrid ? TEXT("OK") : TEXT("NULL"),
		MaxSlots, HotbarSlots,
		SlotWidgetClass ? *SlotWidgetClass->GetName() : TEXT("NULL"));

	// Always rebuild if requested (grid size may have changed)
	if (InventoryGrid) InventoryGrid->ClearChildren();
	if (HotbarGrid) HotbarGrid->ClearChildren();
	SlotWidgets.Empty();
	SlotWidgets.SetNum(MaxSlots);
	ClearItemDataRowCache();

	// Upper inventory area: slots HotbarSlots .. MaxSlots-1
	for (int32 i = HotbarSlots; i < MaxSlots; ++i)
	{
		const int32 LocalIndex = i - HotbarSlots;
		UInv_ItemSlotWidget* SlotWidget = CreateSlotWidget(
			i, InventoryGrid, LocalIndex / InventoryGridColumns, LocalIndex % InventoryGridColumns);
		SlotWidgets[i] = SlotWidget;
	}

	// Lower hotbar area: slots 0 .. HotbarSlots-1
	UGridPanel* TargetGrid = HotbarGrid ? HotbarGrid : InventoryGrid;
	for (int32 i = 0; i < HotbarSlots && i < MaxSlots; ++i)
	{
		UInv_ItemSlotWidget* SlotWidget = CreateSlotWidget(
			i, TargetGrid, 0, i % HotbarGridColumns);
		SlotWidgets[i] = SlotWidget;
	}

	UpdateCapacityText();

	// Refresh all slot displays since CreateSlotWidget only sets data, not icons
	RefreshAllSlots();
}

void UInv_InventoryWidget::RebuildGridWithNewSize(int32 NewMaxSlots, int32 NewHotbarSlots)
{
	MaxSlots = NewMaxSlots;
	HotbarSlots = NewHotbarSlots;
	RebuildInventoryGrid();
}

void UInv_InventoryWidget::RefreshAllSlots()
{
	for (int32 i = 0; i < SlotWidgets.Num(); ++i)
	{
		RefreshSlot(i);
	}
}

void UInv_InventoryWidget::RefreshSlot(int32 SlotIndex)
{
	if (!InventoryComponent || !SlotWidgets.IsValidIndex(SlotIndex)) return;
	if (UInv_ItemSlotWidget* SlotWidget = SlotWidgets[SlotIndex])
	{
		RefreshSlotWidget(SlotWidget, SlotIndex);
		if (SelectedSlotIndex == SlotIndex)
		{
			UpdateDetailPanel(SlotIndex);
		}
	}
}

void UInv_InventoryWidget::RefreshSlotWidget(UInv_ItemSlotWidget* SlotWidget, int32 SlotIndex)
{
	if (!InventoryComponent) return;
	const TArray<FInv_ItemInstance>& Items = InventoryComponent->GetItems();

	if (Items.IsValidIndex(SlotIndex) && Items[SlotIndex].IsValid())
	{
		const FInv_ItemInstance& Item = Items[SlotIndex];
		SlotWidget->SetItemData(Item, SlotIndex);
		FInv_ItemDataRow ItemDataRow;
		if (GetCachedItemDataRow(Item.ItemID, ItemDataRow))
		{
			UTexture2D* Icon = GetItemIcon(Item.ItemID);
			UE_LOG(LogTemp, Log, TEXT("[Inv][UI] 刷新格子 %d: %s x%d Icon=%s"),
				SlotIndex, *Item.ItemID.ToString(), Item.Quantity,
				Icon ? *Icon->GetName() : TEXT("NULL"));
			SlotWidget->UpdateSlotDisplay(Icon, ItemDataRow.ItemName, Item.Quantity);
		}
		else
		{
			//数据表行缺失：回退显示物品ID，便于排查显示问题
			UE_LOG(LogTemp, Warning, TEXT("[Inv][UI] 找不到物品数据行 %s (ItemDataTable=%s)"),
				*Item.ItemID.ToString(),
				InventoryComponent->ItemDataTable ? *InventoryComponent->ItemDataTable->GetName() : TEXT("NULL"));
			SlotWidget->UpdateSlotDisplay(nullptr, FText::FromName(Item.ItemID), Item.Quantity);
		}
	}
	else
	{
		SlotWidget->SetItemData(FInv_ItemInstance::EmptySlot(), SlotIndex);
		SlotWidget->ClearSlotDisplay();
	}
}

void UInv_InventoryWidget::SelectSlot(int32 SlotIndex)
{
	if (!SlotWidgets.IsValidIndex(SlotIndex)) return;

	// If clicking the same slot, deselect it (toggle off)
	if (SelectedSlotIndex == SlotIndex)
	{
		ClearSelection();
		return;
	}

	// If clicking an empty slot, deselect
	if (!InventoryComponent || !InventoryComponent->GetItems().IsValidIndex(SlotIndex) || !InventoryComponent->GetItems()[SlotIndex].IsValid())
	{
		ClearSelection();
		return;
	}

	if (!DetailPanel || !InventoryComponent)
	{
		SelectedSlotIndex = -1;
		return;
	}

	SelectedSlotIndex = SlotIndex;
	UpdateDetailPanel(SlotIndex);
	OnSlotSelected(SlotIndex);
}

void UInv_InventoryWidget::ClearSelection()
{
	OnSlotSelected(-1);
	if (DetailIcon) DetailIcon->SetBrushFromTexture(nullptr);
	if (DetailNameText) DetailNameText->SetText(FText::GetEmpty());
	if (DetailDescriptionText) DetailDescriptionText->SetText(FText::GetEmpty());
	if (DetailQuantityText) DetailQuantityText->SetVisibility(ESlateVisibility::Collapsed);
	SelectedSlotIndex = -1;
	if (DetailPanel)
	{
		DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UInv_InventoryWidget::UpdateDetailPanel(int32 SlotIndex)
{
	if (!DetailPanel || !InventoryComponent) return;

	const TArray<FInv_ItemInstance>& Items = InventoryComponent->GetItems();
	if (!Items.IsValidIndex(SlotIndex) || Items[SlotIndex].IsEmpty())
	{
		DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const FInv_ItemInstance& Item = Items[SlotIndex];
	FInv_ItemDataRow ItemDataRow;
	if (!GetCachedItemDataRow(Item.ItemID, ItemDataRow))
	{
		DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	DetailPanel->SetVisibility(ESlateVisibility::Visible);
	if (DetailIcon)
	{
		DetailIcon->SetBrushFromTexture(GetItemIcon(Item.ItemID));
	}
	if (DetailNameText) DetailNameText->SetText(ItemDataRow.ItemName);
	if (DetailDescriptionText) DetailDescriptionText->SetText(ItemDataRow.ItemDescription);
	if (DetailQuantityText)
	{
		if (ItemDataRow.CanStack())
		{
			DetailQuantityText->SetText(FText::Format(
				NSLOCTEXT("Inventory", "QuantityFormat", "Quantity: {0}"),
				FText::AsNumber(Item.Quantity)));
			DetailQuantityText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			DetailQuantityText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UInv_InventoryWidget::OnInventoryUpdated(const TArray<FInv_ItemInstance>& Items, int32 ChangedSlotIndex)
{
	//诊断：客户端UI事件是否触发
	UE_LOG(LogTemp, Log, TEXT("[Inv][UI] 收到背包刷新事件 (%s) ChangedSlot=%d 有效物品数=%d"),
		InventoryComponent ? *InventoryComponent->GetName() : TEXT("NULL"),
		ChangedSlotIndex, CountValidItems(Items));

	if (ChangedSlotIndex == -1)
	{
		RefreshAllSlots();
	}
	else if (SlotWidgets.IsValidIndex(ChangedSlotIndex))
	{
		RefreshSlot(ChangedSlotIndex);
	}
	UpdateCapacityText();
}

int32 UInv_InventoryWidget::CountValidItems(const TArray<FInv_ItemInstance>& InItems)
{
	int32 Count = 0;
	for (const FInv_ItemInstance& Item : InItems)
	{
		if (Item.IsValid()) Count++;
	}
	return Count;
}

void UInv_InventoryWidget::HandleSlotDragDropSwap(int32 FromSlotIndex, int32 ToSlotIndex)
{
	if (InventoryComponent)
	{
		InventoryComponent->SwapItems(FromSlotIndex, ToSlotIndex);
	}
}

void UInv_InventoryWidget::DropItemFromSlot(int32 SlotIndex)
{
	if (!InventoryComponent) return;

	const TArray<FInv_ItemInstance>& Items = InventoryComponent->GetItems();
	if (!Items.IsValidIndex(SlotIndex) || !Items[SlotIndex].IsValid())
	{
		return;
	}

	const FInv_ItemInstance& DroppedItem = Items[SlotIndex];

	// 在玩家面前生成世界掉落物
	AGAS_PlayerController* PC = Cast<AGAS_PlayerController>(GetOwningPlayer());
	if (PC)
	{
		// 计算生成位置：玩家位置 + 面向方向 * 200cm
		FVector SpawnLocation = FVector::ZeroVector;
		FRotator SpawnRotation = FRotator::ZeroRotator;
		if (APawn* PlayerPawn = PC->GetPawn())
		{
			const FVector Forward = PlayerPawn->GetActorForwardVector();
			SpawnLocation = PlayerPawn->GetActorLocation() + Forward * 200.f;
			SpawnLocation.Z += 50.f;
			SpawnRotation = PlayerPawn->GetActorRotation();
		}

		PC->SpawnDroppedItem(DroppedItem.ItemID, DroppedItem.Quantity, SpawnLocation, SpawnRotation);
	}

	// 从背包移除该堆叠
	InventoryComponent->RemoveItemAtSlot(SlotIndex, DroppedItem.Quantity);

	// 刷新该槽位显示
	RefreshSlot(SlotIndex);

	// 如果丢弃的是当前选中的槽位，清除选中状态
	if (SelectedSlotIndex == SlotIndex)
	{
		ClearSelection();
	}
}

void UInv_InventoryWidget::DropItemsFromSlot(int32 SlotIndex, int32 Quantity)
{
	if (!InventoryComponent) return;

	// 在请求数量时，检查该槽位当前是否有物品
	const TArray<FInv_ItemInstance>& Items = InventoryComponent->GetItems();
	if (!Items.IsValidIndex(SlotIndex) || !Items[SlotIndex].IsValid())
	{
		return;
	}

	// 限制数量最大为槽位中的数量
	int32 ActualQuantity = FMath::Clamp(Quantity, 1, Items[SlotIndex].Quantity);

	// 在玩家面前生成世界掉落物
	AGAS_PlayerController* PC = Cast<AGAS_PlayerController>(GetOwningPlayer());
	if (PC)
	{
		FVector SpawnLocation = FVector::ZeroVector;
		FRotator SpawnRotation = FRotator::ZeroRotator;
		if (APawn* PlayerPawn = PC->GetPawn())
		{
			const FVector Forward = PlayerPawn->GetActorForwardVector();
			SpawnLocation = PlayerPawn->GetActorLocation() + Forward * 200.f;
			SpawnLocation.Z += 50.f;
			SpawnRotation = PlayerPawn->GetActorRotation();
		}

		PC->SpawnDroppedItem(Items[SlotIndex].ItemID, ActualQuantity, SpawnLocation, SpawnRotation);
	}

	// 从槽位移除指定数量的物品
	InventoryComponent->RemoveItemAtSlot(SlotIndex, ActualQuantity);

	// 刷新该槽位显示
	RefreshSlot(SlotIndex);

	// 如果丢弃的是当前选中的槽位，清除选中状态
	if (SelectedSlotIndex == SlotIndex)
	{
		ClearSelection();
	}
}

void UInv_InventoryWidget::RequestDropWithQuantity(int32 SlotIndex)
{
	if (!InventoryComponent) return;

	const TArray<FInv_ItemInstance>& Items = InventoryComponent->GetItems();
	if (!Items.IsValidIndex(SlotIndex) || !Items[SlotIndex].IsValid())
	{
		return;
	}

	// 触发蓝图事件，让 WBP 弹出数量选择器
	OnRequestDropQuantity(SlotIndex, Items[SlotIndex].Quantity);
}

int32 UInv_InventoryWidget::GetUsedSlotCount() const
{
	return InventoryComponent ? InventoryComponent->GetUsedSlotsCount() : 0;
}

void UInv_InventoryWidget::UpdateCapacityText()
{
	if (CapacityText)
	{
		CapacityText->SetText(FText::Format(
			NSLOCTEXT("Inventory", "CapacityFormat", "{0}/{1}"),
			FText::AsNumber(GetUsedSlotCount()),
			FText::AsNumber(MaxSlots)));
	}
}