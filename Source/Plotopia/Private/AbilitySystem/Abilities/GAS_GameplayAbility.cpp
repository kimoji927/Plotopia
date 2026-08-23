// ZhouZun


#include "AbilitySystem/Abilities/GAS_GameplayAbility.h"


void UGAS_GameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (bDrawDebugs && IsValid(GEngine))
	{
		GEngine->AddOnScreenDebugMessage(-1,3.0f,FColor::Cyan,FString::Printf(TEXT("%s Activated"),*GetName()));
	}
}
