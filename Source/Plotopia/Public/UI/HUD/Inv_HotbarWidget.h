// Inv_HotbarWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/Data/Inv_ItemData.h"
#include "Inv_HotbarWidget.generated.h"

// Forward declare to use GetSelectedHotbarSlot()
class UInv_InventoryComponent;
class UHorizontalBox;
class UTextBlock;

UCLASS()
class PLOTOPIA_API UInv_HotbarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Hotbar|UI")
	void InitHotbar(UInv_InventoryComponent* InInventoryComponent);

	UFUNCTION(BlueprintCallable, Category = "Hotbar|UI")
	void RefreshAllSlots();

	UFUNCTION(BlueprintCallable, Category = "Hotbar|UI")
	void RefreshSlot(int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category = "Hotbar|UI")
	UTexture2D* GetItemIcon(FName ItemID);

protected:
	UFUNCTION()
	void OnInventorySlotUpdated(const TArray<FInv_ItemInstance>& Items, int32 SlotIndex);

	UFUNCTION()
	void OnSelectionChanged(const TArray<FInv_ItemInstance>& Items, int32 SlotIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Hotbar|UI")
	void UpdateSlotDisplay(int32 SlotIndex, UTexture2D* Icon, const FText& ItemName, int32 Quantity);

	UFUNCTION(BlueprintImplementableEvent, Category = "Hotbar|UI")
	void ClearSlotDisplay(int32 SlotIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Hotbar|UI")
	void HighlightSelectedSlot(int32 SlotIndex);

	UFUNCTION(BlueprintNativeEvent, Category = "Hotbar|UI")
	bool GetItemDataRow(FName ItemID, FInv_ItemDataRow& OutRow);
	virtual bool GetItemDataRow_Implementation(FName ItemID, FInv_ItemDataRow& OutRow);

	/** Internal: get or cache item data row */
	bool GetCachedItemDataRow(FName ItemID, FInv_ItemDataRow& OutRow);

	UPROPERTY()
	TObjectPtr<UInv_InventoryComponent> InventoryComponent;

	UPROPERTY(Transient)
	TMap<FName, FInv_ItemDataRow> ItemDataRowCache;

private:
	int32 PreviousSelectedSlot = -1;
	/** 上一次处理的组件数据版本号（轮询刷新用，不依赖动态委托） */
	int32 LastSeenVersion = -1;
};