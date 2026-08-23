// ZhouZun

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GAS_GameplayAbility.h"
#include "GAS_HitReact.generated.h"

/**
 * 
 */
UCLASS()
class PLOTOPIA_API UGAS_HitReact : public UGAS_GameplayAbility
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable,Category="GAS|Abilities")
	void CacheHitDirectionVectors(AActor* Instigator);
	
	UPROPERTY(BlueprintReadOnly,Category="GAS|Abilities")
	FVector AvatarForward;
	
	UPROPERTY(BlueprintReadOnly,Category="GAS|Abilities")
	FVector ToInstigator;
};
