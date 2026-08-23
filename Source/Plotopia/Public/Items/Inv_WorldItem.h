// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inv_WorldItem.generated.h"

class UInv_ItemComponent;
class UInv_InventoryComponent;
class UStaticMeshComponent;
class UDataTable;
class FLifetimeProperty;

/**
 * Generic world item actor - used for placed, spawned, and dropped pickups.
 * Mesh is set at runtime from the DataTable (WorldMesh field).
 */
UCLASS()
class PLOTOPIA_API AInv_WorldItem : public AActor
{
	GENERATED_BODY()
	
public:
	AInv_WorldItem();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Item ID (matches DT_Items row name). Set this when placing in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", ReplicatedUsing = OnRep_ItemID)
	FName ItemID;

	/** Item quantity carried by this world item */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", Replicated)
	int32 Quantity = 1;

	/**
	 * Optional DataTable (DT_Items). When set, mesh is previewed in-editor.
	 * When empty, falls back to the player's inventory DataTable at runtime.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TObjectPtr<UDataTable> ItemDataTable;

	/** Initialize this world item with data (also syncs to component) */
	UFUNCTION(BlueprintCallable, Category = "Items|Drop")
	void InitWorldItem(FName InItemID, int32 InQuantity);

	/** Set the static mesh from the item's DataTable entry */
	UFUNCTION(BlueprintCallable, Category = "Items|Drop")
	void SetWorldMeshFromDataTable(UDataTable* InDataTable);

protected:
	/** Sync actor properties to component on construction / placement */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Auto-setup mesh from player's inventory DataTable when spawned in level */
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Items|Drop")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Items|Drop")
	TObjectPtr<UInv_ItemComponent> ItemComponent;

private:
	/** Get the player's inventory component to access the item DataTable */
	UInv_InventoryComponent* GetPlayerInventoryComponent() const;

	/** Sync actor properties (ItemID/Quantity) to the item component */
	void SyncToComponent();

	/** Find the item DataTable from the player's inventory component */
	UDataTable* FindItemDataTable() const;

	/** Retry mesh setup in case the player's inventory isn't ready yet */
	void RetryMeshSetup();

	/** ItemID复制到客户端后刷新网格（服务器生成的掉落物客户端可见） */
	UFUNCTION()
	void OnRep_ItemID();

	/** Timer handle for retry logic */
	FTimerHandle MeshSetupRetryTimer;

	/** 网格设置重试计数（防止玩家背包未就绪时无限重试） */
	int32 MeshSetupRetryCount = 0;

	static constexpr int32 MaxMeshSetupRetries = 10;

};
