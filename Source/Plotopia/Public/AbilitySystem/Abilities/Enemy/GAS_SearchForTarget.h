// ZhouZun

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GAS_GameplayAbility.h"
#include "GAS_SearchForTarget.generated.h"

namespace EPathFollowingResult
{
	enum Type : int;
}
class AGAS_BaseCharacter;
class UGAS_WaitGameplayEvent;
class AAIController;
class AGAS_EnemyCharacter;
class UAbilityTask_WaitDelay;
class UAITask_MoveTo;
/**
 * 
 */
UCLASS()
class PLOTOPIA_API UGAS_SearchForTarget : public UGAS_GameplayAbility
{
	GENERATED_BODY()
public:
	UGAS_SearchForTarget();
	//激活能力
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	TWeakObjectPtr<AGAS_EnemyCharacter> Enemy;
	TWeakObjectPtr<AAIController> EnemyAIController;
	TWeakObjectPtr<AGAS_BaseCharacter> Target;
private:
	UPROPERTY()
	TObjectPtr<UGAS_WaitGameplayEvent> WaitGameplayEventTask;
	
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> SearchDelayTask;
	
	UPROPERTY()
	TObjectPtr<UAITask_MoveTo> MoveToLocationOrActorTask;
	
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> AttackDelayTask;
	
	void StartSearch();
	
	UFUNCTION()
	void EndAttackEventReceived(FGameplayEventData Payload);
	
	UFUNCTION()
	void Search();
	
	void MoveToTargetAndAttack();
	
	UFUNCTION()
	void AttackTarget(TEnumAsByte<EPathFollowingResult::Type> Result,AAIController* AIController);
	
	UFUNCTION()
	void Attack();
};
