// Inv_InventoryWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/Data/Inv_ItemData.h"
#include "Inv_InventoryWidget.generated.h"

class UInv_InventoryComponent;
class UInv_ItemSlotWidget;
class UGridPanel;
class UImage;
class UTextBlock;
class UCanvasPanel;
class UBorder;

UCLASS()
class PLOTOPIA_API UInv_InventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    void InitInventory(UInv_InventoryComponent* InInventoryComponent);

    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    void ToggleInventory();

    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    void ShowInventory();

    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    void HideInventory();

    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    bool IsInventoryVisible() const { return bIsVisible; }

    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    void RebuildInventoryGrid();

    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    void RefreshAllSlots();

    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    void RefreshSlot(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    void SelectSlot(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    void ClearSelection();

    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    void DropItemFromSlot(int32 SlotIndex);

    /** Drop a specified quantity from a slot (used by quantity selector UI) */
    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    void DropItemsFromSlot(int32 SlotIndex, int32 Quantity);

    /** Request a quantity-selection popup for dropping items (triggered by right-click) */
    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    void RequestDropWithQuantity(int32 SlotIndex);

    /** Blueprint event: Called when user right-clicks a slot with items, to show quantity selector */
    UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|UI")
    void OnRequestDropQuantity(int32 SlotIndex, int32 MaxQuantity);

    UFUNCTION(BlueprintPure, Category = "Inventory|UI")
    int32 GetSelectedSlotIndex() const { return SelectedSlotIndex; }

    UFUNCTION(BlueprintPure, Category = "Inventory|UI")
    int32 GetUsedSlotCount() const;

    /** Blueprint event: Called when a slot is selected/deselected. SlotIndex = -1 means deselected. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|UI")
    void OnSlotSelected(int32 SlotIndex);

    UFUNCTION(BlueprintPure, Category = "Inventory|UI")
    int32 GetMaxSlotCount() const { return MaxSlots; }

    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    UTexture2D* GetItemIcon(FName ItemID);

    UFUNCTION(BlueprintNativeEvent, Category = "Inventory|UI")
    bool GetItemDataRow(FName ItemID, FInv_ItemDataRow& OutRow);
    virtual bool GetItemDataRow_Implementation(FName ItemID, FInv_ItemDataRow& OutRow);

    UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
    UInv_InventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

    /** Clear cached data and rebuild the grid. Call when inventory size changes. */
    void RebuildGridWithNewSize(int32 NewMaxSlots, int32 NewHotbarSlots);

    virtual void NativeDestruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    /** 上一次处理的组件数据版本号（轮询刷新用，不依赖动态委托） */
    int32 LastSeenVersion = -1;

    /** Helper: create and configure a single slot widget (used by both inventory and hotbar grids) */
    UInv_ItemSlotWidget* CreateSlotWidget(int32 SlotIndex, UGridPanel* TargetGrid, int32 Row, int32 Column);
    UFUNCTION()
    void OnInventoryUpdated(const TArray<FInv_ItemInstance>& Items, int32 ChangedSlotIndex);

    /** 统计数组中的有效物品数（诊断用） */
    static int32 CountValidItems(const TArray<FInv_ItemInstance>& InItems);

    UFUNCTION()
    void HandleSlotDragDropSwap(int32 FromSlotIndex, int32 ToSlotIndex);

    void UpdateDetailPanel(int32 SlotIndex);
    void UpdateCapacityText();
    void RefreshSlotWidget(UInv_ItemSlotWidget* SlotWidget, int32 SlotIndex);

    UPROPERTY()
    TObjectPtr<UInv_InventoryComponent> InventoryComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|State")
    bool bIsVisible = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|State")
    int32 SelectedSlotIndex = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
    int32 MaxSlots = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
    int32 HotbarSlots = 9;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
    int32 InventoryGridColumns = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
    int32 HotbarGridColumns = 9;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Config")
    TSubclassOf<UInv_ItemSlotWidget> SlotWidgetClass;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCanvasPanel> MainPanel;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> InventoryTitleText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UGridPanel> InventoryGrid;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UGridPanel> HotbarGrid;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> CapacityText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UBorder> DetailPanel;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> DetailIcon;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> DetailNameText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> DetailDescriptionText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> DetailQuantityText;

    UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI")
    TArray<TObjectPtr<UInv_ItemSlotWidget>> SlotWidgets;

    /** Cache for item data rows to avoid repeated DataTable lookups */
    UPROPERTY(Transient)
    TMap<FName, FInv_ItemDataRow> ItemDataRowCache;

    /** 已加载的物品图标缓存（避免每次刷新都 LoadSynchronous 软引用） */
    UPROPERTY(Transient)
    TMap<FName, TObjectPtr<UTexture2D>> ItemIconCache;


    /** Retrieve item data row with caching */
    bool GetCachedItemDataRow(FName ItemID, FInv_ItemDataRow& OutRow);

    /** Clear the item data row cache */
    void ClearItemDataRowCache();
};
