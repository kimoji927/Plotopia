// ZhouZun


#include "UI/GAS_AttributeWidget.h"

void UGAS_AttributeWidget::OnAttributeChange(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair,UGAS_AttributeSet* AttributeSet, float OldValue)
{
	//从AttributeSet中获取该传入属性的实时数值
	const float AttributeValue = Pair.Key.GetNumericValue(AttributeSet);
	const float MaxAttributeValue = Pair.Value.GetNumericValue(AttributeSet);
	
	//调用蓝图中实现的方法(设置百分比)
	BP_OnAttributeChange(AttributeValue,MaxAttributeValue,OldValue); 
}

bool UGAS_AttributeWidget::MatchesAttribute(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const
{
	//检查传入的T元组类型的值是否与我们蓝图里设置的属性匹配(Key:键的值=属性值，Value:对的值=最大值)
	return Attribute == Pair.Key && MaxAttribute == Pair.Value;
}
