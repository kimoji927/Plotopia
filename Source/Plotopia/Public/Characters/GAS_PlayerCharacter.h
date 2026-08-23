// ZhouZun

#pragma once

#include "CoreMinimal.h"
#include "GAS_BaseCharacter.h"
#include "GAS_PlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;

UCLASS()
class PLOTOPIA_API AGAS_PlayerCharacter : public AGAS_BaseCharacter
{
	GENERATED_BODY()

public:
	AGAS_PlayerCharacter();
	
	//重写GetAbilitySystemComponent()虚函数
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//重写GetAttributeSet()虚函数
	virtual UAttributeSet* GetAttributeSet() const override;
	
	//Pawn被服务器上的Controller控制后，在服务器需要执行的初始化逻辑
	virtual void PossessedBy(AController* NewController) override;
	//服务器上的PlayerState发生变化同步到客户端后，引擎在客户端自动调用该函数
	virtual void OnRep_PlayerState() override;

protected:
	/** 重写死亡处理：死亡时把背包内全部物品散落在地面 */
	virtual void HandleDeath() override;

private:
	/** 死亡掉落：把背包内全部有效物品生成世界掉落物（仅服务器执行，客户端忽略） */
	void DropInventoryItemsOnDeath();

	UPROPERTY(VisibleAnywhere,Category="Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;
	
	UPROPERTY(VisibleAnywhere,Category="Camera")
	TObjectPtr<UCameraComponent> FollowCamera;
};
