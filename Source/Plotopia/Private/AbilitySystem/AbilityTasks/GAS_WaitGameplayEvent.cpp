// ZhouZun


#include "AbilitySystem/AbilityTasks/GAS_WaitGameplayEvent.h"

UGAS_WaitGameplayEvent* UGAS_WaitGameplayEvent::WaitGameplayEventToActorProxy(AActor* TargetActor,
	FGameplayTag EventTag, bool OnlyTriggerOnce, bool OnlyMatchExact)
{
	UGAS_WaitGameplayEvent* MyObj = NewObject<UGAS_WaitGameplayEvent>();
	MyObj->SetAbilityActor(TargetActor);
	MyObj->Tag = EventTag;
	MyObj->OnlyTriggerOnce = OnlyTriggerOnce;
	MyObj->OnlyMatchExact = OnlyMatchExact;
	return MyObj;
}

void UGAS_WaitGameplayEvent::StartActivation()
{
	Activate();
}
