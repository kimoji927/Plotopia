// ZhouZun


#include "AbilitySystem/Abilities/Player/GAS_Primary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTags/GASTags.h"


void UGAS_Primary::SendHitReactEventToActor(const TArray<AActor*>& ActorsHit)
{
	for (AActor* HitActor : ActorsHit)
	{
		FGameplayEventData Payload;
		//事件的发起者
		Payload.Instigator = GetAvatarActorFromActorInfo();
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitActor,GASTags::GASEvents::Enemy::HitReact,Payload);
	}
}

