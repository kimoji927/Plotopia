// ZhouZun

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GAS_AttributeSet.h"
#include "Blueprint/UserWidget.h"
#include "GAS_AttributeWidget.generated.h"

UCLASS()
class PLOTOPIA_API UGAS_AttributeWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	//定义游戏属性类型的变量，在蓝图里设置这些变量
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="GAS|Attributes")
	FGameplayAttribute Attribute;
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="GAS|Attributes")
	FGameplayAttribute MaxAttribute;
	
	//监听属性变化时的回调函数(属性变化时执行这个函数),传入GAS_AttributeSet访问该属性集
	void OnAttributeChange(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair,UGAS_AttributeSet* AttributeSet,float OldValue);
	
	//检查传入的T元组是否与该小部件蓝图里设置的属性匹配
	bool MatchesAttribute(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const;
	
	//C++中声明,蓝图中实现的函数
	UFUNCTION(BlueprintImplementableEvent,meta = (DisplayName = "On Attribute Change"))
	void BP_OnAttributeChange(float NewValue,float NewMaxValue,float OldValue);
	
	UPROPERTY(BlueprintReadOnly,Category="GAS|Attributes")
	TWeakObjectPtr<AActor> AvatarActor;
};
