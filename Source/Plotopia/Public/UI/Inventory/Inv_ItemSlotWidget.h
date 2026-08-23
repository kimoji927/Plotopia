// Inv_ItemSlotWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Items/Data/Inv_ItemData.h"
#include "Inv_ItemSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UInv_InventoryWidget;

DECLARE_DELEGATE_TwoParams(FOnSlotDragDropSwap, int32, int32);

UCLASS()
class PLOTOPIA_API UInv_ItemSlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    void SetItemData(const FInv_ItemInstance& InItemData, int32 InSlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    void SetParentInventory(UInv_InventoryWidget* InParentInventory);

    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    const FInv_ItemInstance& GetItemData() const { return ItemData; }

    /** Get the parent inventory widget (callable from Blueprint) */
    UFUNCTION(BlueprintPure, Category = "Inventory|UI")
    UInv_InventoryWidget* GetParentInventoryWidget() const;

    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    int32 GetSlotIndex() const { return SlotIndex; }

    /** Set slot index when dynamically creating slot widgets */
    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    void SetSlotIndex(int32 NewIndex) { SlotIndex = NewIndex; }

    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    bool HasItem() const { return ItemData.IsValid(); }

    /** Blueprint event: Update slot display with icon, name, and quantity */
    UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|UI")
    void UpdateSlotDisplay(UTexture2D* Icon, const FText& ItemName, int32 Quantity);

    /** Blueprint event: Clear slot display (shows empty slot) */
    UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|UI")
    void ClearSlotDisplay();

    /** Blueprint event: Called when the slot is clicked (left-click, no drag) */
    UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|UI")
    void OnSlotClicked();

    /** Blueprint event: Called when right-clicked */
    UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|UI")
    void OnRightClicked();

    /** Blueprint event: Called when a drag operation hovers over this slot */
    UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|DragDrop")
    void OnDragOverSlot();

    /** Blueprint event: Called when mouse enters this slot (hover) */
    UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|UI")
    void OnMouseEnterSlot();

    /** Blueprint event: Called when mouse leaves this slot */
    UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|UI")
    void OnMouseLeaveSlot();

    /** Blueprint event: Create a separate widget for drag visual */
    UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|DragDrop")
    UUserWidget* CreateDragVisualWidget();

    /** Set the drag-drop swap callback */
    void SetOnSlotSwapCallback(const FOnSlotDragDropSwap& InCallback) { OnSlotDragDropSwap = InCallback; }

protected:
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
    virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    UPROPERTY(meta = (BindWidget), BlueprintReadWrite)
    TObjectPtr<UBorder> SlotBorder;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|UI")
    FInv_ItemInstance ItemData;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|UI")
    int32 SlotIndex = -1;

    UPROPERTY()
    TObjectPtr<UInv_InventoryWidget> ParentInventory;

    FOnSlotDragDropSwap OnSlotDragDropSwap;

    UPROPERTY(Transient)
    bool bMouseDownOnSlot = false;
};
