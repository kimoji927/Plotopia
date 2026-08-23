// ZhouZun


#include "AbilitySystem/GAS_AttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectExtension.h"
#include "GameplayTags/GASTags.h"
#include "Net/UnrealNetwork.h"

void UGAS_AttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	//无条件复制，并且总是触发，属性必须有ReplicatedUsing = OnRep_...标签
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass,Health,COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass,MaxHealth,COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass,Mana,COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass,MaxMana,COND_None,REPNOTIFY_Always);
	
	//有了一个复制的bool值
	DOREPLIFETIME(ThisClass,bAttributesInitialized);
}

//首次效果应用后
void UGAS_AttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(),0.f,GetMaxHealth()));
	}
	
	if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(),0.f,GetMaxMana()));
	}
	
	if (Data.EvaluatedData.Attribute == GetHealthAttribute() && GetHealth() <= 0.f)
	{
		FGameplayEventData Payload;
		//注意：此处语义为 Instigator=阵亡者、Target=击杀者，事件发送给击杀者(玩家)。
		//与GAS惯例(Instigator=施加者)相反，如需统一请同步修改 GA_ListenForKillScored 蓝图的读取端。
		Payload.Instigator = Data.Target.GetAvatarActor();
		Payload.Target = Data.EffectSpec.GetEffectContext().GetInstigator();
		//发送游戏事件
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Data.EffectSpec.GetEffectContext().GetInstigator(),GASTags::GASEvents::KillScored,Payload);
	}
	
	//如果属性未初始化
	if (!bAttributesInitialized)
	{
		bAttributesInitialized = true;
		//发送广播
		OnAttributesInitialized.Broadcast();
	}
	
}

void UGAS_AttributeSet::OnRep_AttributesInitialized()
{
	//如果已属性初始化
	if (bAttributesInitialized)
	{
		//发送广播
		OnAttributesInitialized.Broadcast();
	}
}

void UGAS_AttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	//通知GAS系统该属性已复制，以便处理预测回滚
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass,Health,OldValue);
}

void UGAS_AttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass,MaxHealth,OldValue);
}

void UGAS_AttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass,Mana,OldValue);
}

void UGAS_AttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass,MaxMana,OldValue);
}
