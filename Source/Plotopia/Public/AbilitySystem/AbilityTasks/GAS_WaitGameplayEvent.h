// ZhouZun

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Async/AbilityAsync_WaitGameplayEvent.h"
#include "GAS_WaitGameplayEvent.generated.h"

/**
 * 
 */
UCLASS()
class PLOTOPIA_API UGAS_WaitGameplayEvent : public UAbilityAsync_WaitGameplayEvent
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable,Category="Ability|Async",meta=(DefaultToSelf = "TargetActor",BlueprintInternalUseOnly = "TRUE"))
	static UGAS_WaitGameplayEvent* WaitGameplayEventToActorProxy(AActor* TargetActor,UPARAM(meta=(GameplayTagFilter="GameplayEventCategory")) FGameplayTag EventTag,bool OnlyTriggerOnce=false,bool OnlyMatchExact = true);
	
	void StartActivation();
};
