// ZhouZun

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Components/WidgetComponent.h"
#include "GAS_WidgetComponent.generated.h"

class UAbilitySystemComponent;
class UGAS_AttributeSet;
class UGAS_AbilitySystemComponent;
class AGAS_BaseCharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PLOTOPIA_API UGAS_WidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere)
	TMap<FGameplayAttribute,FGameplayAttribute> AttributeMap;
private:
	TWeakObjectPtr<AGAS_BaseCharacter> GASCharacter;
	TWeakObjectPtr<UGAS_AbilitySystemComponent> AbilitySystemComponent;
	TWeakObjectPtr<UGAS_AttributeSet> AttributeSet;
	
	//初始化能力数据(设置三个弱对象指针)
	void InitAbilitySystemData();
	//检查这三个指针是否已经初始化
	bool IsASCInitialized() const;
	//初始化属性委托
	void InitializeAttributeDelegate();
	
	void BindWidgetToAttributeChanges(UWidget* widgetObject,const TTuple<FGameplayAttribute,FGameplayAttribute>& Pair) const;
	
	//要绑定到动态多播委托的回调函数
	UFUNCTION()
	void OnASCInitialized(UAbilitySystemComponent* ASC,UAttributeSet* AS);
	
	//要绑定到属性初始化委托的回调
	UFUNCTION()
	void BindToAttributeChanges();
};
