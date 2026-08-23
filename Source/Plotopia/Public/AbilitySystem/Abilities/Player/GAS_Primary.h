// ZhouZun

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GAS_GameplayAbility.h"
#include "GAS_Primary.generated.h"

/**
 * 
 */
UCLASS()
class PLOTOPIA_API UGAS_Primary : public UGAS_GameplayAbility
{
	GENERATED_BODY()
public:
	
	UFUNCTION(BlueprintCallable,Category="GAS|Abilities")
	void SendHitReactEventToActor(const TArray<AActor*>& ActorsHit);
	
protected:
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="GAS|Abilities")
	float HitBoxRadius = 100.0f;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="GAS|Abilities")
	float HitBoxForwardOffest = 200.0f;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="GAS|Abilities")
	float HitBoxElevationOffest = 20.0f;

};
