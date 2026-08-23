// ZhouZun


#include "Characters/GAS_PlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/GAS_AttributeSet.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Items/Components/Inv_InventoryComponent.h"
#include "Player/GAS_PlayerController.h"
#include "Player/GAS_PlayerState.h"


// Sets default values
AGAS_PlayerCharacter::AGAS_PlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	
	//设置胶囊体碰撞体积
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	//角色朝向移动方向移动
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 500.0f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.0f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>("CameraBoom");
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 600.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 120.0f, 60.0f);
	CameraBoom->bUsePawnControlRotation = true;
	
	FollowCamera = CreateDefaultSubobject<UCameraComponent>("FollowCamera");
	FollowCamera->SetupAttachment(CameraBoom,USpringArmComponent::SocketName);
	//别管鼠标怎么动，你就死死抱住你父级组件（通常是弹簧臂）的大腿，它怎么转你就怎么转。
	FollowCamera->bUsePawnControlRotation = false;
	
	Tags.Add(CrashTags::Player);
}

UAbilitySystemComponent* AGAS_PlayerCharacter::GetAbilitySystemComponent() const
{
	AGAS_PlayerState* GASPlayerState = Cast<AGAS_PlayerState>(GetPlayerState());
	if (!IsValid(GASPlayerState))return nullptr;
	
	return GASPlayerState->GetAbilitySystemComponent();
}

UAttributeSet* AGAS_PlayerCharacter::GetAttributeSet() const
{
	AGAS_PlayerState* GASPlayerState = Cast<AGAS_PlayerState>(GetPlayerState());
	if (!IsValid(GASPlayerState))return nullptr;
	
	return GASPlayerState->GetAttributeSet();
}

void AGAS_PlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	if (!IsValid(GetAbilitySystemComponent()) || !HasAuthority())return;
	
	GetAbilitySystemComponent()->InitAbilityActorInfo(GetPlayerState(),this);
	//发送委托,广播(AbilitySystemComponent和AttributeSet的值)
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(),GetAttributeSet());
	
	GiveStartupAbilities();
	
	InitializeAttributes();
	
	UGAS_AttributeSet* GAS_AttributeSet = Cast<UGAS_AttributeSet>(GetAttributeSet());
	if (!IsValid(GAS_AttributeSet)) return;
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(GAS_AttributeSet->GetHealthAttribute()).AddUObject(this,&ThisClass::OnHealthChanged);
}

void AGAS_PlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	if (!IsValid(GetAbilitySystemComponent()))return;
	
	GetAbilitySystemComponent()->InitAbilityActorInfo(GetPlayerState(),this);
	//发送委托,广播(AbilitySystemComponent和AttributeSet的值)
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(),GetAttributeSet());
	
	UGAS_AttributeSet* GAS_AttributeSet = Cast<UGAS_AttributeSet>(GetAttributeSet());
	if (!IsValid(GAS_AttributeSet)) return;
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(GAS_AttributeSet->GetHealthAttribute()).AddUObject(this,&ThisClass::OnHealthChanged);
}

void AGAS_PlayerCharacter::HandleDeath()
{
	Super::HandleDeath();

	// 死亡时把背包内全部物品掉落在地面（HandleDeath 在服务器与客户端都会执行，
	// 但生成世界掉落物只能由服务器权威执行，客户端调用会直接忽略）
	DropInventoryItemsOnDeath();
}

void AGAS_PlayerCharacter::DropInventoryItemsOnDeath()
{
	// 只有服务器能生成世界掉落物
	if (!HasAuthority()) return;

	AGAS_PlayerController* PC = Cast<AGAS_PlayerController>(GetController());
	if (!IsValid(PC)) return;

	UInv_InventoryComponent* Inventory = PC->GetInventoryComponent();
	if (!IsValid(Inventory)) return;

	const TArray<FInv_ItemInstance>& Items = Inventory->GetItems();
	if (Items.Num() == 0) return;

	const FVector DeathLocation = GetActorLocation();
	const FRotator DeathRotation = GetActorRotation();

	int32 DroppedCount = 0;
	for (const FInv_ItemInstance& Item : Items)
	{
		if (!Item.IsValid()) continue;

		// 在死亡位置周围随机散布，避免所有物品重叠在一点
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float Radius = FMath::FRandRange(80.f, 200.f);
		FVector DropLocation = DeathLocation;
		DropLocation.X += FMath::Cos(Angle) * Radius;
		DropLocation.Y += FMath::Sin(Angle) * Radius;
		DropLocation.Z += 40.f;

		PC->SpawnDroppedItem(Item.ItemID, Item.Quantity, DropLocation, DeathRotation);
		++DroppedCount;
	}

	if (DroppedCount > 0)
	{
		// 物品已全部掉落，清空背包（服务器权威，客户端UI随之刷新为空）
		Inventory->ClearInventory();
		UE_LOG(LogTemp, Log, TEXT("[Inv] 玩家 %s 死亡，掉落 %d 组物品"), *GetName(), DroppedCount);
	}
}


