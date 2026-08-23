// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/Inv_WorldItem.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Items/Components/Inv_InventoryComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Items/Data/Inv_ItemData.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "TimerManager.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"

AInv_WorldItem::AInv_WorldItem()
{
	PrimaryActorTick.bCanEverTick = false;

	//服务器运行时生成的掉落物需要复制到所有客户端
	bReplicates = true;

	// Mesh component with item trace collision + real physics
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));

	// Use the custom 'Item' collision profile (configured in Project Settings > Collision)
	// Profile: ObjectType=WorldDynamic, Block=WorldStatic, Ignore=Pawn, Block=ItemTrace
	MeshComponent->SetCollisionProfileName(TEXT("Item"));
	MeshComponent->SetGenerateOverlapEvents(true);
	RootComponent = MeshComponent;

	// Item component for interaction/pickup
	ItemComponent = CreateDefaultSubobject<UInv_ItemComponent>(TEXT("Inv_Item"));
}

void AInv_WorldItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ItemID);
	DOREPLIFETIME(ThisClass, Quantity);
}

void AInv_WorldItem::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Sync actor-level properties to the item component so they appear in the world correctly
	SyncToComponent();

	// If in-editor and components are initialized, try to preview the mesh too
	// Use the optional ItemDataTable property first, then fall back to player's
	if (ItemComponent && ItemComponent->ItemID != NAME_None && MeshComponent)
	{
		UDataTable* DT = ItemDataTable ? ItemDataTable.Get() : FindItemDataTable();
		if (DT)
		{
			SetWorldMeshFromDataTable(DT);
		}
	}
}

void AInv_WorldItem::InitWorldItem(FName InItemID, int32 InQuantity)
{
	ItemID = InItemID;
	Quantity = FMath::Max(1, InQuantity);

	SyncToComponent();
}

void AInv_WorldItem::BeginPlay()
{
	Super::BeginPlay();

	// Sync actor-level properties to the component (works for both placed & spawned items)
	SyncToComponent();

	// Auto-setup mesh: use optional property first, fall back to player's inventory DataTable
	if (UDataTable* DT = ItemDataTable ? ItemDataTable.Get() : FindItemDataTable())
	{
		SetWorldMeshFromDataTable(DT);
	}
	else if (!ItemDataTable)
	{
		// Player's inventory not ready yet (PC might still be initializing) - retry shortly
		GetWorldTimerManager().SetTimer(MeshSetupRetryTimer, this, &AInv_WorldItem::RetryMeshSetup, 0.2f, true);
	}
}

void AInv_WorldItem::RetryMeshSetup()
{
	// Stop retrying once we have a DataTable
	if (UDataTable* DT = FindItemDataTable())
	{
		SetWorldMeshFromDataTable(DT);
		GetWorldTimerManager().ClearTimer(MeshSetupRetryTimer);
		return;
	}

	//达到最大重试次数后放弃，避免永远重试
	if (++MeshSetupRetryCount >= MaxMeshSetupRetries)
	{
		GetWorldTimerManager().ClearTimer(MeshSetupRetryTimer);
	}
}

void AInv_WorldItem::OnRep_ItemID()
{
	//服务器生成的掉落物复制到客户端后，根据ItemID刷新网格，保证客户端可见
	if (ItemID == NAME_None || !MeshComponent) return;

	if (UDataTable* DT = ItemDataTable ? ItemDataTable.Get() : FindItemDataTable())
	{
		SetWorldMeshFromDataTable(DT);
	}
	else
	{
		//玩家背包数据表尚未就绪，稍后重试
		GetWorldTimerManager().SetTimer(MeshSetupRetryTimer, this, &AInv_WorldItem::RetryMeshSetup, 0.2f, true);
	}
}

UInv_InventoryComponent* AInv_WorldItem::GetPlayerInventoryComponent() const
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			// InventoryComponent is created on the PlayerController (via NewObject)
			if (UInv_InventoryComponent* InvComp = PC->FindComponentByClass<UInv_InventoryComponent>())
			{
				return InvComp;
			}

			// Fallback: check Pawn as well (alternative setups)
			if (APawn* Pawn = PC->GetPawn())
			{
				return Pawn->FindComponentByClass<UInv_InventoryComponent>();
			}
		}
	}
	return nullptr;
}

void AInv_WorldItem::SyncToComponent()
{
	if (ItemComponent)
	{
		ItemComponent->ItemID = ItemID;
		ItemComponent->Quantity = FMath::Max(1, Quantity);
	}
}

UDataTable* AInv_WorldItem::FindItemDataTable() const
{
	if (UInv_InventoryComponent* InvComp = GetPlayerInventoryComponent())
	{
		return InvComp->ItemDataTable;
	}
	return nullptr;
}

void AInv_WorldItem::SetWorldMeshFromDataTable(UDataTable* InDataTable)
{
	if (ItemID == NAME_None || !MeshComponent) return;
	if (!InDataTable) return;

	static const FString ContextStr(TEXT("SetWorldMeshFromDataTable"));
	if (FInv_ItemDataRow* Row = InDataTable->FindRow<FInv_ItemDataRow>(ItemID, ContextStr))
	{
		if (!Row->WorldMesh.IsNull())
		{
			UStaticMesh* Mesh = Row->WorldMesh.LoadSynchronous();
			if (Mesh)
			{
				MeshComponent->SetStaticMesh(Mesh);
				MeshComponent->SetRelativeScale3D(FVector(1.0f)); // World-appropriate scale

				// Enable real physics: gravity + collision with the world
				MeshComponent->SetSimulatePhysics(true);
				MeshComponent->SetEnableGravity(true);
				MeshComponent->SetMassScale(NAME_None, 1.0f);
			}
		}
	}
}
