// ZhouZun

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Items/Data/Inv_ItemData.h"
#include "GAS_PlayerController.generated.h"

class UInv_HUDWidget;
class UInv_InventoryComponent;
class UInv_InventoryWidget;
class UInv_HotbarWidget;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
struct FGameplayTag;
struct FKey;

UCLASS()
class PLOTOPIA_API AGAS_PlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AGAS_PlayerController();
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UInv_InventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UInv_InventoryWidget* GetInventoryWidget() const { return InventoryWidget; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UInv_HUDWidget* GetHUDWidget() const { return HUDWidget; }

	/** Spawn a world item into the world (used when discarding from inventory) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	AActor* SpawnDroppedItem(FName ItemID, int32 Quantity, const FVector& DropLocation, const FRotator& DropRotation = FRotator::ZeroRotator);

	// ==================== 快捷栏直接使用（不用打开背包） ====================
	/**
	 * 使用当前“选中快捷栏槽位”里的物品（消耗品）。
	 * 默认绑定鼠标右键（在 SetupInputComponent 里绑定，无需配置任何输入资源）；
	 * 若想在 BP 里改成别的键/手柄键：给 UseItemAction 指定一个 InputAction 并映射按键即可，
	 * 指定后就走增强输入，鼠标右键的默认绑定自动失效。
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Consumable")
	bool UseSelectedHotbarItem();

	/** 按索引选中快捷栏槽位（数字键 1~9 调用；也可在蓝图/UI 里直接调用） */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Hotbar")
	void SelectHotbarSlotByIndex(int32 SlotIndex);

	/**
	 * 服务器→所属客户端：推送完整背包状态（可靠）。
	 * 动态创建在PC上的组件属性复制有时序问题（客户端组件可能晚于服务器初始复制
	 * 创建，导致客户端背包永远收不到数据），此RPC作为保证通道。
	 */
	UFUNCTION(Client, Reliable)
	void Client_ReceiveInventoryState(const TArray<FInv_ItemInstance>& InItems, int32 InMaxSlots, int32 InHotbarCount);

protected:
	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;

private:
	UPROPERTY(EditDefaultsOnly,Category="GAS|Input|Movement")
	TArray<TObjectPtr<UInputMappingContext>> InputMappingContexts;

	UPROPERTY(EditDefaultsOnly,Category="GAS|Input|Inventory")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly,Category="GAS|Input|Inventory")
	TObjectPtr<UInputAction> ToggleInventoryAction;

	UPROPERTY(EditDefaultsOnly,Category="GAS|Input|Movement")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly,Category="GAS|Input|Movement")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly,Category="GAS|Input|Movement")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly,Category="GAS|Input|Abilities")
	TObjectPtr<UInputAction> PrimaryAction;

	UPROPERTY(EditDefaultsOnly,Category="GAS|Input|Abilities")
	TObjectPtr<UInputAction> SecondaryAction;

	UPROPERTY(EditDefaultsOnly,Category="GAS|Input|Abilities")
	TObjectPtr<UInputAction> TertiaryAction;

	UPROPERTY(EditDefaultsOnly,Category="GAS|Input|Hotbar")
	TObjectPtr<UInputAction> MouseWheelAction;

	/** 可选：快捷栏“使用物品”的输入动作。留空 = 默认绑定鼠标右键 */
	UPROPERTY(EditDefaultsOnly,Category="GAS|Input|Hotbar")
	TObjectPtr<UInputAction> UseItemAction;

	/** 输入回调（输入绑定要求返回 void） */
	void OnUseItemInput();

	/** 数字键 1~9 选择快捷栏槽位（同一个处理函数按 FKey 分发，参数按委托签名传值） */
	void OnHotbarNumberKey(FKey Key);

	void Interact();
	void TraceForItem();

	/** 服务器端实际拾取逻辑（含防作弊距离校验） */
	void PickupItem(AActor* TargetItem);

	/** 客户端发起拾取：路由到服务器执行 */
	UFUNCTION(Server, Reliable)
	void Server_InteractWithItem(AActor* TargetItem);

	/** 客户端发起丢弃：路由到服务器生成世界物品 */
	UFUNCTION(Server, Reliable)
	void Server_SpawnDroppedItem(FName ItemID, int32 Quantity, const FVector& DropLocation, const FRotator& DropRotation);

	UPROPERTY(EditDefaultsOnly,Category="GAS|Inventory")
	double TraceLength;

	UPROPERTY(EditDefaultsOnly,Category="GAS|Inventory")
	TEnumAsByte<ECollisionChannel> ItemTraceChannel;
	TWeakObjectPtr<AActor> ThisActor;
	TWeakObjectPtr<AActor> LastActor;

	//物品追踪节流：不必每帧做射线检测
	UPROPERTY(EditDefaultsOnly,Category="GAS|Inventory")
	float TraceInterval{0.1f};

	UPROPERTY()
	float TimeSinceLastTrace{0.f};

	void Jump();
	void StopJumping();
	void Look(const FInputActionValue& Value);
	void Move(const FInputActionValue& Value);

	void Primary();
	void Secondary();
	void Tertiary();

	//能力激活函数
	void ActivateAbility(const FGameplayTag& AbilityTag) const;
	bool IsAlive() const;

	UPROPERTY(EditDefaultsOnly,Category="GAS|Inventory")
	TSubclassOf<UInv_HUDWidget> HUDWidgetClass;

	UPROPERTY()
	TObjectPtr<UInv_HUDWidget> HUDWidget;

	void CreateHUDWidget();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory",meta = (AllowPrivateAccess="true"))
	TObjectPtr<UInv_InventoryComponent> InventoryComponent;

	// ===== 服务器RPC推送状态暂存（防止RPC早于组件创建时丢失初始数据，且避免重复创建组件）=====
	UPROPERTY(Transient)
	TArray<FInv_ItemInstance> PendingInventoryItems;

	UPROPERTY(Transient)
	int32 PendingMaxSlots = 0;

	UPROPERTY(Transient)
	int32 PendingHotbarCount = 0;

	UPROPERTY(Transient)
	bool bHasPendingInventoryState = false;

	UPROPERTY(EditDefaultsOnly,Category="GAS|Inventory")
	TSubclassOf<UInv_InventoryWidget> InventoryWidgetClass;

	UPROPERTY()
	TObjectPtr<UInv_InventoryWidget> InventoryWidget;

	void CreateInventoryComponent();
	void CreateInventoryWidget();
	void ToggleInventory();

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Inventory")
	int32 MaxSlots = 30;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Inventory")
	int32 HotbarSlotCount = 9;

	UPROPERTY(EditDefaultsOnly,Category="GAS|Inventory")
	TObjectPtr<class UDataTable> ItemDataTable;

	//====快捷栏系统====
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Hotbar")
	TSubclassOf<UInv_HotbarWidget> HotbarWidgetClass;

	UPROPERTY()
	TObjectPtr<UInv_HotbarWidget> HotbarWidget;

	void CreateHotbarWidget();

	void OnMouseWheel(const FInputActionValue& Value);
	void HotbarSelectNext();
	void HotbarSelectPrevious();
};
