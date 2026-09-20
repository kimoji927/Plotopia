// Inv_ItemSlotWidget.cpp
#include "UI/Inventory/Inv_ItemSlotWidget.h"
#include "UI/Inventory/Inv_InventoryWidget.h"
#include "UI/Inventory/Inv_DragDropOperation.h"
#include "Items/Components/Inv_InventoryComponent.h"
#include "Player/GAS_PlayerController.h"
#include "Components/Border.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

UInv_InventoryWidget* UInv_ItemSlotWidget::GetParentInventoryWidget() const
{
	// 常规路径：由 C++ CreateSlotWidget 创建的格子（背包网格）会显式设置父级
	if (ParentInventory)
	{
		return ParentInventory;
	}

	// 回退路径：由蓝图自行创建、从未调用 SetParentInventory 的格子（例如 WBP_Hotbar
	// 在蓝图里创建的快捷栏格子）会拿到空父级，导致客户端点击/悬停/拖拽时
	// GetParentInventoryWidget() 返回空并报蓝图运行时错误。这里回退到
	// 拥有者玩家控制器上的背包 UI，保证任何机器的行为一致。
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AGAS_PlayerController* GASPC = Cast<AGAS_PlayerController>(PC))
		{
			return GASPC->GetInventoryWidget();
		}
	}
	return nullptr;
}

UInv_InventoryComponent* UInv_ItemSlotWidget::GetInventoryComponent() const
{
	// 优先父级背包UI（C++创建的格子），其次拥有者玩家控制器上的背包组件
	if (UInv_InventoryWidget* Parent = GetParentInventoryWidget())
	{
		if (UInv_InventoryComponent* InvComp = Parent->GetInventoryComponent())
		{
			return InvComp;
		}
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AGAS_PlayerController* GASPC = Cast<AGAS_PlayerController>(PC))
		{
			return GASPC->GetInventoryComponent();
		}
	}
	return nullptr;
}

bool UInv_ItemSlotWidget::IsItemConsumable() const
{
	if (!ItemData.IsValid()) return false;

	if (UInv_InventoryComponent* InvComp = GetInventoryComponent())
	{
		return InvComp->IsConsumableItem(ItemData.ItemID);
	}
	return false;
}

void UInv_ItemSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // 鼠标悬停时触发高亮
    OnMouseEnterSlot();
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
}

void UInv_ItemSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    // 鼠标离开时取消悬停高亮
    OnMouseLeaveSlot();
    Super::NativeOnMouseLeave(InMouseEvent);
}

void UInv_ItemSlotWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UInv_ItemSlotWidget::NativeDestruct()
{
    OnSlotDragDropSwap.Unbind();
    ParentInventory = nullptr;
    Super::NativeDestruct();
}

void UInv_ItemSlotWidget::SetItemData(const FInv_ItemInstance& InItemData, int32 InSlotIndex)
{
    ItemData = InItemData;
    SlotIndex = InSlotIndex;
}

void UInv_ItemSlotWidget::SetParentInventory(UInv_InventoryWidget* InParentInventory)
{
    ParentInventory = InParentInventory;
}

FReply UInv_ItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        bMouseDownOnSlot = true;
        FReply Reply = FReply::Handled();
        // Only capture and enable drag when there is an item
        if (ItemData.IsValid())
        {
            Reply.CaptureMouse(TakeWidget());
            Reply.DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
        }
        else
        {
            // Let mouse up handle selection clearing
            Reply.CaptureMouse(TakeWidget());
        }
        return Reply;
    }
    else if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        // 处理右键 Down 事件，确保右键 Up 能够到达此控件
        return FReply::Handled();
    }
    return FReply::Unhandled();
}

FReply UInv_ItemSlotWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // 如果鼠标按下过且没有发生拖拽，则触发左键点击事件
        if (bMouseDownOnSlot)
        {
            OnSlotClicked();
        }
        bMouseDownOnSlot = false;
        return FReply::Handled().ReleaseMouseCapture();
    }
    else if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        if (ItemData.IsValid())
        {
            // 右键分流：
            //   消耗品（数据行勾选 bIsConsumable）→ 走GAS使用流程（应用配置的GameplayEffect回血等，并扣除物品）
            //   非消耗品 → 保持原有行为：弹出数量选择器丢弃
            // 注意：消耗品分支不因“使用失败”而回退到丢弃框，否则主机(有权限)与客户端(只发RPC)
            // 的表现会不一致。
            bool bHandledByUse = false;
            if (IsItemConsumable())
            {
                if (UInv_InventoryComponent* InvComp = GetInventoryComponent())
                {
                    bHandledByUse = true;
                    // 客户端返回true表示请求已发往服务器；服务器端返回true表示本次使用成功
                    const bool bUsed = InvComp->UseItemAtSlot(SlotIndex);
                    OnItemUsed(bUsed);
                }
            }

            if (!bHandledByUse)
            {
                // 非消耗品：通知父背包弹出数量选择器
                if (UInv_InventoryWidget* ParentWidget = GetParentInventoryWidget())
                {
                    ParentWidget->RequestDropWithQuantity(SlotIndex);
                }
            }

            // 同时保留蓝图事件，方便 WBP 额外处理
            OnRightClicked();
        }
        return FReply::Handled();
    }
    return FReply::Unhandled();
}

void UInv_ItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    OutOperation = nullptr;
    if (!ItemData.IsValid()) return;
    if (!IsValid(ParentInventory)) return;

    UInv_DragDropOperation* InvDragOp = Cast<UInv_DragDropOperation>(
        UWidgetBlueprintLibrary::CreateDragDropOperation(UInv_DragDropOperation::StaticClass()));
    if (!InvDragOp) return;

    InvDragOp->DragPayload.FromSlotIndex = SlotIndex;
    InvDragOp->DragPayload.DraggedItem = ItemData;
    InvDragOp->DragPayload.DragIcon = ParentInventory->GetItemIcon(ItemData.ItemID);

    if (UUserWidget* DragVisual = CreateDragVisualWidget())
    {
        InvDragOp->DefaultDragVisual = DragVisual;
    }
    InvDragOp->Pivot = EDragPivot::CenterCenter;
    OutOperation = InvDragOp;
    bMouseDownOnSlot = false;
}

void UInv_ItemSlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    // 拖拽取消时，确保取消所有高亮
    //OnDragLeaveSlot();
    bMouseDownOnSlot = false;
}

bool UInv_ItemSlotWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    if (Cast<UInv_DragDropOperation>(InOperation))
    {
        // 鼠标拖拽经过时触发高亮
        OnDragOverSlot();
        return true;
    }
    return false;
}

bool UInv_ItemSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    UInv_DragDropOperation* InvDragOp = Cast<UInv_DragDropOperation>(InOperation);
    if (!InvDragOp) return false;

    const int32 FromSlotIndex = InvDragOp->DragPayload.FromSlotIndex;
    if (FromSlotIndex < 0 || FromSlotIndex == SlotIndex) return false;

    // 拖拽放下时取消所有高亮
    //OnDragLeaveSlot();

    // Ensure parent inventory and component are still valid
    if (OnSlotDragDropSwap.IsBound() && IsValid(ParentInventory))
    {
        OnSlotDragDropSwap.Execute(FromSlotIndex, SlotIndex);
    }
    return true;
}