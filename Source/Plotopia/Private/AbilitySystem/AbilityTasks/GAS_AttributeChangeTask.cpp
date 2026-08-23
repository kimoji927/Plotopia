// ZhouZun


#include "AbilitySystem/AbilityTasks/GAS_AttributeChangeTask.h"
#include "AbilitySystemComponent.h"

UGAS_AttributeChangeTask* UGAS_AttributeChangeTask::ListenForAttributeChange(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAttribute Attribute)
{
	//创建UGAS_AttributeChangeTask这个类类型的实例
	UGAS_AttributeChangeTask* WaitForAttributeChangeTask = NewObject<UGAS_AttributeChangeTask>();
	WaitForAttributeChangeTask->ASC = AbilitySystemComponent;
	WaitForAttributeChangeTask->AttributeToListenFor = Attribute;

	if (!IsValid(AbilitySystemComponent))
	{
		WaitForAttributeChangeTask->RemoveFromRoot();
		return nullptr;
	}
	
	//当游戏属性变化时，执行AttributeChanged
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(WaitForAttributeChangeTask,&UGAS_AttributeChangeTask::AttributeChanged);
	return WaitForAttributeChangeTask;
}

void UGAS_AttributeChangeTask::EndTask()
{
	if (ASC.IsValid())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(AttributeToListenFor).RemoveAll(this);
	}
	SetReadyToDestroy();
	MarkAsGarbage();
}

void UGAS_AttributeChangeTask::AttributeChanged(const FOnAttributeChangeData& Data)
{
	//广播传递Data
	OnAttributeChanged.Broadcast(Data.Attribute,Data.NewValue,Data.OldValue);
}
