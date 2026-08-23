// ZhouZun

#include "Player/GAS_PlayerController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Characters/GAS_BaseCharacter.h"
#include "GameFramework/Character.h"
#include "GameplayTags/GASTags.h"
#include "Items/Components/Inv_InventoryComponent.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Items/Inv_WorldItem.h"
#include "Kismet/GameplayStatics.h"
#include "UI/HUD/Inv_HUDWidget.h"
#include "UI/HUD/Inv_HotbarWidget.h"
#include "UI/Inventory/Inv_InventoryWidget.h"

AGAS_PlayerController::AGAS_PlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	TraceLength = 500.0;
	//蓝图中不指定默认是ECC_GameTraceChannel1碰撞通道
	ItemTraceChannel = ECC_GameTraceChannel1;
}

void AGAS_PlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	//服务器端远程玩家的PC不需要物品追踪（拾取由客户端RPC发起）
	if (!IsLocalController()) return;
	
	//物品追踪节流：按固定间隔做射线检测，避免每帧一条射线
	TimeSinceLastTrace += DeltaTime;
	if (TimeSinceLastTrace >= TraceInterval)
	{
		TimeSinceLastTrace = 0.f;
		TraceForItem();
	}
}

void AGAS_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	//背包组件必须在增强输入初始化之前创建：
	//服务器端远程玩家的PC没有LocalPlayer（InputSubsystem为空），
	//但服务器必须拥有背包组件作为权威数据源，否则远程玩家无法拾取/复制物品。
	CreateInventoryComponent();

	//本地玩家：配置增强输入映射（服务器端自动跳过）
	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* Context : InputMappingContexts)
		{
			InputSubsystem->AddMappingContext(Context,0);
		}
	}

	//本地玩家：创建UI（各函数内部已做IsLocalController判断，服务器端自动跳过）
	CreateHUDWidget();
	CreateInventoryWidget();
	CreateHotbarWidget();
}

void AGAS_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!IsValid(EnhancedInputComponent)) return;

	EnhancedInputComponent->BindAction(InteractAction,ETriggerEvent::Started,this,&ThisClass::Interact);
	EnhancedInputComponent->BindAction(ToggleInventoryAction,ETriggerEvent::Started,this,&ThisClass::ToggleInventory);
	EnhancedInputComponent->BindAction(JumpAction,ETriggerEvent::Started,this,&ThisClass::Jump);
	EnhancedInputComponent->BindAction(JumpAction,ETriggerEvent::Completed,this,&ThisClass::StopJumping);
	EnhancedInputComponent->BindAction(LookAction,ETriggerEvent::Triggered,this,&ThisClass::Look);
	EnhancedInputComponent->BindAction(MoveAction,ETriggerEvent::Triggered,this,&ThisClass::Move);

	EnhancedInputComponent->BindAction(PrimaryAction,ETriggerEvent::Triggered,this,&ThisClass::Primary);
	EnhancedInputComponent->BindAction(SecondaryAction,ETriggerEvent::Started,this,&ThisClass::Secondary);
	EnhancedInputComponent->BindAction(TertiaryAction,ETriggerEvent::Started,this,&ThisClass::Tertiary);

	//滚轮切换快捷栏
	EnhancedInputComponent->BindAction(MouseWheelAction, ETriggerEvent::Triggered, this, &ThisClass::OnMouseWheel);
}

void AGAS_PlayerController::Interact()
{
	if (!ThisActor.IsValid()) return;

	// 死亡后不能拾取（防止把死亡时掉落的物品立即捡回）
	if (!IsAlive()) return;

	//客户端拾取：路由到服务器执行（服务器权威 + 物品由服务器销毁并复制）
	if (!HasAuthority())
	{
		Server_InteractWithItem(ThisActor.Get());
		return;
	}

	PickupItem(ThisActor.Get());
}

void AGAS_PlayerController::Server_InteractWithItem_Implementation(AActor* TargetItem)
{
	PickupItem(TargetItem);
}

void AGAS_PlayerController::PickupItem(AActor* TargetItem)
{
	if (!IsValid(TargetItem)) return;
	if (!IsValid(InventoryComponent)) return;

	UInv_ItemComponent* ItemComponent = TargetItem->FindComponentByClass<UInv_ItemComponent>();
	if (!IsValid(ItemComponent) || ItemComponent->ItemID == NAME_None) return;

	//防作弊：限制拾取距离（基于追踪长度 + 余量）
	if (APawn* MyPawn = GetPawn())
	{
		const double MaxPickupDistSq = FMath::Square(TraceLength + 200.0);
		if (FVector::DistSquared(MyPawn->GetActorLocation(), TargetItem->GetActorLocation()) > MaxPickupDistSq) return;
	}

	// 使用掉落物上的数量（如果是旧版没有设置的，默认 1）
	int32 PickupQuantity = FMath::Max(1, ItemComponent->Quantity);
	int32 Added = InventoryComponent->AddItem(ItemComponent->ItemID, PickupQuantity);
	if (Added > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("拾取了物品: %s (ID: %s, Qty: %d)"), *TargetItem->GetName(), *ItemComponent->ItemID.ToString(), Added);

		// If partially picked up (backpack filled), update remaining quantity on the dropped item
		if (Added < PickupQuantity)
		{
			ItemComponent->Quantity = PickupQuantity - Added;
			UE_LOG(LogTemp, Warning, TEXT("背包空间不足，剩余 %d 个留在原地"), ItemComponent->Quantity);
		}
		else
		{
			if (IsValid(HUDWidget)) HUDWidget->HidePickUpMessage();
			AActor* Target = TargetItem;
			ThisActor = nullptr;  // Clear reference before destroy to prevent re-entry
			if (IsValid(Target))
			{
				Target->Destroy();
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("背包已满，无法拾取: %s"), *ItemComponent->ItemID.ToString());
	}
}

void AGAS_PlayerController::TraceForItem()
{
	// 死亡后不显示拾取提示
	if (!IsAlive())
	{
		if (IsValid(HUDWidget)) HUDWidget->HidePickUpMessage();
		return;
	}

	if (!IsValid(GEngine) || !IsValid(GEngine->GameViewport)) return;
	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);
	const FVector2D ViewportCenter = ViewportSize / 2.f;
	FVector	TraceStart;
	FVector Forward;
	if (!UGameplayStatics::DeprojectScreenToWorld(this,ViewportCenter,TraceStart,Forward)) return;
	const FVector TraceEnd = TraceStart + Forward * TraceLength;
	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult,TraceStart,TraceEnd,ItemTraceChannel);

	LastActor = ThisActor;
	ThisActor = HitResult.GetActor();
	//如果这个物品无效
	if (!ThisActor.IsValid())
	{
		//检查小部件是否有效，并隐藏
		if (IsValid(HUDWidget)) HUDWidget->HidePickUpMessage();
	}
	//检查当前这个物品是否是上一帧射线检测到的物品，如果是则直接返回
	if (ThisActor == LastActor) return;
	//如果当前这个物品有效
	if (ThisActor.IsValid())
	{
		UInv_ItemComponent* ItemComponent = ThisActor->FindComponentByClass<UInv_ItemComponent>();
		//如果这个物品没有挂载UInv_ItemComponent则直接返回
		if (!IsValid(ItemComponent)) return;
		//如果该物品挂载了UInv_ItemComponent,则显示(组件上的PickupMessage变量)拾取提示
		if (IsValid(HUDWidget)) HUDWidget->ShowPickUpMessage(ItemComponent->GetPickupMessage());
	}
}

AActor* AGAS_PlayerController::SpawnDroppedItem(FName ItemID, int32 Quantity, const FVector& DropLocation, const FRotator& DropRotation)
{
	//客户端丢弃：路由到服务器生成世界物品（世界物品由服务器权威）
	if (!HasAuthority())
	{
		Server_SpawnDroppedItem(ItemID, Quantity, DropLocation, DropRotation);
		return nullptr;
	}

	if (ItemID == NAME_None || Quantity <= 0) return nullptr;

	// Spawn generic dropped item actor
	FActorSpawnParameters Params;
	Params.Owner = GetPawn();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AInv_WorldItem* WorldItem = GetWorld()->SpawnActor<AInv_WorldItem>(AInv_WorldItem::StaticClass(), DropLocation, DropRotation, Params);
	if (!WorldItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn world item: %s"), *ItemID.ToString());
		return nullptr;
	}

	// Initialize with item data (sets ItemID + Quantity on component)
	WorldItem->InitWorldItem(ItemID, Quantity);

	// Set world mesh from the item DataTable
	WorldItem->SetWorldMeshFromDataTable(ItemDataTable);

	UE_LOG(LogTemp, Log, TEXT("World item spawned: %s (ID: %s, Qty: %d)"), *WorldItem->GetName(), *ItemID.ToString(), Quantity);
	return WorldItem;
}

void AGAS_PlayerController::Server_SpawnDroppedItem_Implementation(FName ItemID, int32 Quantity, const FVector& DropLocation, const FRotator& DropRotation)
{
	SpawnDroppedItem(ItemID, Quantity, DropLocation, DropRotation);
}

void AGAS_PlayerController::Client_ReceiveInventoryState_Implementation(const TArray<FInv_ItemInstance>& InItems, int32 InMaxSlots, int32 InHotbarCount)
{
	// 客户端接收服务器推送的背包状态。
	// 注意：这里绝不允许创建背包组件 —— 组件的创建只能由 BeginPlay 路径完成，
	// 否则会与 BeginPlay 产生竞态，在同一个PC上创建出两个组件，破坏复制匹配。
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->ApplyReplicatedState(InItems, InMaxSlots, InHotbarCount);
	}
	else
	{
		// 组件尚未创建（RPC 可能早于 BeginPlay 到达）：暂存状态，
		// 待 CreateInventoryComponent 创建组件后补应用。
		PendingInventoryItems = InItems;
		PendingMaxSlots = InMaxSlots;
		PendingHotbarCount = InHotbarCount;
		bHasPendingInventoryState = true;
	}
}

void AGAS_PlayerController::Jump()
{
	if (!IsValid(GetCharacter())) return;
	if (!IsAlive()) return;
	GetCharacter()->Jump();
}

void AGAS_PlayerController::StopJumping()
{
	if (!IsValid(GetCharacter())) return;
	if (!IsAlive()) return;
	GetCharacter()->StopJumping();
}

void AGAS_PlayerController::Look(const FInputActionValue& Value)
{
	if (!IsAlive()) return;
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddYawInput(LookAxisVector.X);
	AddPitchInput(LookAxisVector.Y);
}

void AGAS_PlayerController::Move(const FInputActionValue& Value)
{
	if (!IsValid(GetPawn())) return;
	if (!IsAlive()) return;
	const FVector2D MovementVector = Value.Get<FVector2D>();

	//找哪里是前
	const FRotator YawRotation(0.f,GetControlRotation().Yaw,0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	GetPawn()->AddMovementInput(ForwardDirection,MovementVector.Y);
	GetPawn()->AddMovementInput(RightDirection,MovementVector.X);
}

void AGAS_PlayerController::Primary()
{
	ActivateAbility(GASTags::GASAbilities::Player::Primary);
}

void AGAS_PlayerController::Secondary()
{
	ActivateAbility(GASTags::GASAbilities::Player::Secondary);
}

void AGAS_PlayerController::Tertiary()
{
	ActivateAbility(GASTags::GASAbilities::Player::Tertiary);
}

void AGAS_PlayerController::ActivateAbility(const FGameplayTag& AbilityTag) const
{
	if (!IsAlive()) return;
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
	if (!IsValid(ASC))return;
	//通过标签激活能力
	ASC->TryActivateAbilitiesByTag(AbilityTag.GetSingleTagContainer());
}

void AGAS_PlayerController::CreateHUDWidget()
{
	//只在本地玩家控制器创建HUD，服务器端不创建
	if (!IsLocalController()) return;
	//通过蓝图中指定的HUDWidgetClass，动态创建一个真正的 UInv_HUDWidget 实例
	HUDWidget = CreateWidget<UInv_HUDWidget>(this,HUDWidgetClass);
	//将小部件添加到视口
	if (IsValid(HUDWidget))
	{
		HUDWidget->AddToViewport();
	}
}

void AGAS_PlayerController::CreateInventoryComponent()
{
	//服务器（权威数据源）与本地玩家（即时UI/输入）都创建；非本地非权威的PC代理无需创建
	if (!HasAuthority() && !IsLocalController()) return;

	// 幂等保护：无论成员指针状态如何（例如曾被置空/对象被回收），都绝不允许
	// 同一个PC上出现第二个背包组件。服务器与客户端必须保持组件名一一对应，
	// 否则复制GUID无法解析，客户端发送RPC时会断开连接（"Client attempted to
	// create sub-object" / "Unable to resolve default guid"）。
	if (IsValid(InventoryComponent)) return;
	TArray<UActorComponent*> Components;
	GetComponents(Components);
	for (UActorComponent* Comp : Components)
	{
		if (!Comp || !Comp->IsA<UInv_InventoryComponent>()) continue;
		if (IsValid(Comp))
		{
			InventoryComponent = CastChecked<UInv_InventoryComponent>(Comp);
			return;
		}
		// 失效的旧组件：销毁并从组件列表移除，避免残留干扰复制
		Comp->DestroyComponent(true);
	}

	InventoryComponent = NewObject<UInv_InventoryComponent>(this, UInv_InventoryComponent::StaticClass());
	if (IsValid(InventoryComponent))
	{
		// 注意：不要调用 AddInstanceComponent —— NewObject 的 PostInitProperties
		// 已把组件加入 OwnedComponents/ReplicatedComponents（注册进复制通道），
		// AddInstanceComponent 会把组件额外加入 InstanceComponents 并改变
		// CreationMethod，反而干扰组件复制匹配。
		InventoryComponent->RegisterComponent();
		// NewObject does NOT trigger BeginPlay(), so we must manually initialize
		InventoryComponent->InitializeInventory(MaxSlots, HotbarSlotCount, ItemDataTable);

		// 若可靠RPC早于组件创建到达（暂存了状态），现在补应用，保证初始数据不丢
		if (bHasPendingInventoryState)
		{
			InventoryComponent->ApplyReplicatedState(PendingInventoryItems, PendingMaxSlots, PendingHotbarCount);
			bHasPendingInventoryState = false;
		}

		UE_LOG(LogTemp, Warning, TEXT("背包组件搭载成功 (手动初始化) %s"), *InventoryComponent->GetName());
	}
}

void AGAS_PlayerController::CreateInventoryWidget()
{
	// 确保必须先有背包组件
	if (!IsValid(InventoryComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateInventoryWidget: 背包组件不存在，无法创建背包UI"));
		return;
	}

	// 只在本地玩家控制器创建背包UI
	if (!IsLocalController()) return;

	// 如果背包UI已存在（例如关卡旅行后仍持有旧实例），需要重新加入视口；
	// NativeConstruct 会负责重新绑定复制事件并刷新，否则旅行后UI会处于脱离视口、
	// 委托解绑的状态，永远收不到背包数据更新。
	if (IsValid(InventoryWidget))
	{
		InventoryWidget->AddToViewport();
		InventoryWidget->HideInventory();
		return;
	}

	// 检查背包UI控件类是否已指定
	if (!IsValid(InventoryWidgetClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateInventoryWidget: InventoryWidgetClass 未在蓝图中指定"));
		return;
	}

	// 动态创建背包UI实例
	InventoryWidget = CreateWidget<UInv_InventoryWidget>(this, InventoryWidgetClass);
	if (IsValid(InventoryWidget))
	{
		InventoryWidget->AddToViewport();
		// AddToViewport 会触发 NativeConstruct，其中通过玩家控制器取得背包组件
		// 并完成 InitInventory（绑定事件 + 重建格子 + 刷新）。这里只需保持隐藏。
		InventoryWidget->HideInventory();
	}
}

void AGAS_PlayerController::ToggleInventory()
{
	// 检查背包组件和背包UI是否存在
	if (!IsValid(InventoryComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("ToggleInventory: 背包组件不存在"));
		return;
	}

	if (!IsValid(InventoryWidget))
	{
		UE_LOG(LogTemp, Warning, TEXT("ToggleInventory: 背包UI不存在，尝试创建..."));
		CreateInventoryWidget();
		// 如果创建后仍然无效，则返回
		if (!IsValid(InventoryWidget)) return;
	}

	// 通过背包UI切换显示/隐藏
	InventoryWidget->ToggleInventory();

	// 根据背包UI的可见性，设置输入模式
	if (InventoryWidget->IsInventoryVisible())
	{
		// 打开背包：隐藏HUD快捷栏（避免和背包内的HotbarGrid重叠）
		if (IsValid(HotbarWidget))
		{
			HotbarWidget->SetVisibility(ESlateVisibility::Collapsed);
		}

		// 显示鼠标，设置输入模式为GameAndUI
		SetShowMouseCursor(true);
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
	else
	{
		// 关闭背包：恢复HUD快捷栏
		if (IsValid(HotbarWidget))
		{
			HotbarWidget->SetVisibility(ESlateVisibility::Visible);
		}

		// 隐藏鼠标，恢复游戏输入模式
		SetShowMouseCursor(false);
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
	}
}

// ===== 快捷栏 =====

void AGAS_PlayerController::CreateHotbarWidget()
{
	// 只在本地玩家控制器创建，服务器端不创建
	if (!IsLocalController()) return;

	// 确保必须有背包组件和UI类
	if (!HotbarWidgetClass || !IsValid(InventoryComponent)) return;

	// 若已存在（例如旅行后残留的旧实例），重新加入视口并重新初始化，保证
	// 复制事件重新绑定、快捷栏数据能正常刷新。
	if (IsValid(HotbarWidget))
	{
		HotbarWidget->InitHotbar(InventoryComponent);
		HotbarWidget->AddToViewport(0);
		return;
	}

	// 动态创建快捷栏UI实例
	HotbarWidget = CreateWidget<UInv_HotbarWidget>(this, HotbarWidgetClass);
	if (IsValid(HotbarWidget))
	{
		HotbarWidget->InitHotbar(InventoryComponent);
		HotbarWidget->AddToViewport(0);
	}
}

void AGAS_PlayerController::OnMouseWheel(const FInputActionValue& Value)
{
	float WheelValue = Value.Get<float>();
	if (WheelValue > 0.0f)
		HotbarSelectPrevious(); // 滚轮向上 → 上一个
	else if (WheelValue < 0.0f)
		HotbarSelectNext();     // 滚轮向下 → 下一个
}

void AGAS_PlayerController::HotbarSelectNext()
{
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->SelectNextHotbarSlot();
	}
}

void AGAS_PlayerController::HotbarSelectPrevious()
{
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->SelectPreviousHotbarSlot();
	}
}

bool AGAS_PlayerController::IsAlive() const
{
	AGAS_BaseCharacter* BaseCharacter = Cast<AGAS_BaseCharacter>(GetPawn());
	if (!IsValid(BaseCharacter)) return false;
	return BaseCharacter->IsAlive();
}
