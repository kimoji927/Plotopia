// ZhouZun


#include "GameObjects/GAS_Projectile.h"
#include "AbilitySystemComponent.h"
#include "Characters/GAS_PlayerCharacter.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayTags/GASTags.h"
#include "Utils/GAS_BlueprintLibrary.h"


AGAS_Projectile::AGAS_Projectile()
{
	PrimaryActorTick.bCanEverTick = false;
	
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	
	bReplicates = true;
}

void AGAS_Projectile::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	
	AGAS_PlayerCharacter* PlayerCharacter = Cast<AGAS_PlayerCharacter>(OtherActor);
	if (!IsValid(PlayerCharacter)) return;
	if (!PlayerCharacter->IsAlive()) return;
	UAbilitySystemComponent* AbilitySystemComponent = PlayerCharacter->GetAbilitySystemComponent();
	if (!IsValid(AbilitySystemComponent) || !HasAuthority()) return;
	
	FGameplayEventData Payload;
	Payload.Instigator = GetOwner();
	Payload.Target = PlayerCharacter;
	
	UGAS_BlueprintLibrary::SendDamageEventToPlayer(PlayerCharacter,DamageEffect,Payload,GASTags::SetByCaller::Projectile,Damage,GASTags::None);
	
	SpawnImpactEffects();
	Destroy(); 
}


