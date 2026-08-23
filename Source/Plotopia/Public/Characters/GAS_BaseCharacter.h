#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GAS_BaseCharacter.generated.h"

namespace CrashTags
{
	extern PLOTOPIA_API const FName Player;
}
struct FOnAttributeChangeData;
class UAttributeSet;
class UGameplayEffect;
class UGameplayAbility;

//声明动态多播委托(传出AbilitySystemComponent和AttributeSet两个参数)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FASCInitialized,UAbilitySystemComponent*,ASC,UAttributeSet*,AS);

//这是一个抽象类，无法被实例化
UCLASS(Abstract)
class PLOTOPIA_API AGAS_BaseCharacter : public ACharacter,public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	//构造函数
	AGAS_BaseCharacter();
	//重写GetLifetimeReplicatedProps函数
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	//创建一个‘返回能力系统指针’的虚函数
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//创建一个‘返回属性集指针’的虚函数
	virtual UAttributeSet* GetAttributeSet() const {return nullptr;}
	
	//获取角色是否存活
	bool IsAlive() const {return bAlive;}
	//设置角色是否存活
	void SetAlive(bool bAliveStatus) {bAlive = bAliveStatus;};
	
	//创建委托(能力系统初始化后)
	UPROPERTY(BlueprintAssignable)
	FASCInitialized OnASCInitialized;
	
	//处理重生
	UFUNCTION(BlueprintCallable, Category = "GAS|Death")
	virtual void HandleRespawn();
	
	//重设属性
	UFUNCTION(BlueprintCallable, Category = "GAS|Death")
	void ResetAttributes();
	
	//旋转朝向目标
	UFUNCTION(BlueprintImplementableEvent)
	void RotateToTarget(AActor* RotateTarget);
	
	//追踪半径
	UPROPERTY(EditAnywhere,Category="GAS|AI")
	float SearchRange{1000.f};
protected:
	void GiveStartupAbilities();
	//创建初始化属性函数
	void InitializeAttributes() const;
	//角色生命值变化
	void OnHealthChanged(const FOnAttributeChangeData& AttributeChangeData);
	//处理死亡
	virtual void HandleDeath();
private:
	//创建能力数组
	UPROPERTY(EditDefaultsOnly,Category="GAS|Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;
	
	UPROPERTY(EditDefaultsOnly,Category="GAS|Effects")
	TSubclassOf<UGameplayEffect> InitializeAttributesEffect;
	
	UPROPERTY(EditDefaultsOnly,Category="GAS|Effects")
	TSubclassOf<UGameplayEffect> ResetAttributesEffect;
	
	//创建可网络复制的bool-是否存活
	UPROPERTY(BlueprintReadOnly,meta=(AllowPrivateAccess="true"),Replicated)
	bool bAlive = true;
};
