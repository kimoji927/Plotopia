// ZhouZun

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GAS_Projectile.generated.h"

class UProjectileMovementComponent;
class UGameplayEffect;

UCLASS()
class PLOTOPIA_API AGAS_Projectile : public AActor
{
	GENERATED_BODY()

public:
	AGAS_Projectile();
	//当任何物体与这个Actor重叠时,触发这个函数
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	//伤害值约定为正数（与ClampMin=0一致），SendDamageEventToPlayer内部会取负应用
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="GAS|Damage",meta = (ExposeOnSpawn,ClampMin = "0.0"))
	float Damage{10.f};
	
	UFUNCTION(BlueprintImplementableEvent,Category="GAS|Projectile")
	void SpawnImpactEffects();
private:
	UPROPERTY(VisibleAnywhere,Category="GAS|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
	
	UPROPERTY(EditDefaultsOnly,Category="GAS|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;
};
