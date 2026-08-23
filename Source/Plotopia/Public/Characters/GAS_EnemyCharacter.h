// ZhouZun

#pragma once

#include "CoreMinimal.h"
#include "GAS_BaseCharacter.h"
#include "GAS_EnemyCharacter.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;

UCLASS()
class PLOTOPIA_API AGAS_EnemyCharacter : public AGAS_BaseCharacter
{
	GENERATED_BODY()

public:
	AGAS_EnemyCharacter();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;
	
	//搜索半径
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="GAS|AI")
	float AcceptanceRadius{500.f};
	
	//最小攻击延迟
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="GAS|AI")
	float MinAttackDelay{.1f};

	//最大攻击延迟
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="GAS|AI")
	float MaxAttackDelay{.5f};
	
	UFUNCTION(BlueprintImplementableEvent)
	float GetTimelineLength();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	bool bIsBeingLaunched{false};
	
	void StopMovementUntilLanded();
protected:
	virtual void BeginPlay() override;
	virtual void HandleDeath() override; 
private:
	UFUNCTION()
	void EnableMovementLanded(const FHitResult& Hit);
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;
};
