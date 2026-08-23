// ZhouZun


#include "AbilitySystem/Abilities/Enemy/GAS_SearchForTarget.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Characters/GAS_EnemyCharacter.h"
#include "Abilities/Async/AbilityAsync_WaitGameplayEvent.h"
#include "AbilitySystem/AbilityTasks/GAS_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Utils/GAS_BlueprintLibrary.h"
#include "GameplayTags/GASTags.h"
#include "Tasks/AITask_MoveTo.h"


UGAS_SearchForTarget::UGAS_SearchForTarget()
{
	//实例化策略
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	//网络执行策略
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	
}

void UGAS_SearchForTarget::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	Enemy = Cast<AGAS_EnemyCharacter>(GetAvatarActorFromActorInfo());
	check(Enemy.IsValid());
	EnemyAIController = Cast<AAIController>(Enemy->GetController());
	check(EnemyAIController.IsValid());
	
	StartSearch();
	
	WaitGameplayEventTask = UGAS_WaitGameplayEvent::WaitGameplayEventToActorProxy(GetAvatarActorFromActorInfo(),GASTags::GASEvents::Enemy::EndAttack);
	WaitGameplayEventTask->EventReceived.AddDynamic(this,&ThisClass::EndAttackEventReceived);
	WaitGameplayEventTask->StartActivation();
}

void UGAS_SearchForTarget::StartSearch()
{
	if (bDrawDebugs) GEngine->AddOnScreenDebugMessage(-1,3.f, FColor::Green,FString::Printf(TEXT("Searching for Target.")));
	if (!Enemy.IsValid()) return;
	
	const float SearchDelay = FMath::RandRange(Enemy->MinAttackDelay,Enemy->MaxAttackDelay);
	SearchDelayTask = UAbilityTask_WaitDelay::WaitDelay(this,SearchDelay);
	SearchDelayTask->OnFinish.AddDynamic(this,&ThisClass::Search);
	SearchDelayTask->Activate();
}

void UGAS_SearchForTarget::EndAttackEventReceived(FGameplayEventData Payload)
{
	if (Enemy.IsValid() && !Enemy->bIsBeingLaunched)
	{
		StartSearch();
	}
}

void UGAS_SearchForTarget::Search()
{
	const FVector Origin = GetAvatarActorFromActorInfo()->GetActorLocation();
	if (!Enemy.IsValid()) return;
	//将GetAvatarActorFromActorInfo()传入GetAllActorsWithTag函数的第一个参数时，系统会自动通过传入的Actor获得这个Actor所在的世界
	FClosestActorWithTagResult ClosestActorResult = UGAS_BlueprintLibrary::FindClosestActorWithTag(GetAvatarActorFromActorInfo(),Origin,CrashTags::Player,Enemy->SearchRange);
	
	Target = Cast<AGAS_BaseCharacter>(ClosestActorResult.Actor);
	
	if (!Target.IsValid())
	{
		StartSearch();
		return;
	}
	if (Target->IsAlive())
	{
		MoveToTargetAndAttack();
	}
	else
	{
		StartSearch();
	}
}

void UGAS_SearchForTarget::MoveToTargetAndAttack()
{
	if (!Enemy.IsValid() || !EnemyAIController.IsValid() || !Target.IsValid()) return;
	if (!Enemy->IsAlive())
	{
		StartSearch();
		return;
	}
	
	MoveToLocationOrActorTask = UAITask_MoveTo::AIMoveTo(EnemyAIController.Get(),FVector(),Target.Get(),Enemy->AcceptanceRadius);
	MoveToLocationOrActorTask->OnMoveTaskFinished.AddUObject(this,&ThisClass::AttackTarget);
	MoveToLocationOrActorTask->ConditionalPerformMove();
}

void UGAS_SearchForTarget::AttackTarget(TEnumAsByte<EPathFollowingResult::Type> Result, AAIController* AIController)
{
	if (Result != EPathFollowingResult::Success)
	{
		StartSearch();
		return;
	}
	Enemy->RotateToTarget(Target.Get());
	
	AttackDelayTask = UAbilityTask_WaitDelay::WaitDelay(this,Enemy->GetTimelineLength());
	AttackDelayTask->OnFinish.AddDynamic(this,&ThisClass::Attack);
	AttackDelayTask->Activate();
}

void UGAS_SearchForTarget::Attack()
{
	const FGameplayTag AttackTag = GASTags::GASAbilities::Enemy::Attack;
	GetAbilitySystemComponentFromActorInfo()->TryActivateAbilitiesByTag(AttackTag.GetSingleTagContainer());
}

