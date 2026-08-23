// ZhouZun


#include "UI/GAS_WidgetComponent.h"

#include "AbilitySystem/GAS_AbilitySystemComponent.h"
#include "AbilitySystem/GAS_AttributeSet.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/GAS_BaseCharacter.h"
#include "UI/GAS_AttributeWidget.h"


void UGAS_WidgetComponent::BeginPlay()
{
	Super::BeginPlay();
	//尝试获取三个指针的值
	InitAbilitySystemData();
	//检查能力系统和属性集指针是否有效
	if (!IsASCInitialized())
	{
		//如果指针无效，将回调函数绑定到两个指针初始化完成时
		GASCharacter->OnASCInitialized.AddDynamic(this,&ThisClass::OnASCInitialized);
		return;
	}
	//如果此时能力系统组件和属性集已初始化，跳过ASC初始化时的委托绑定，直接初始化属性委托
	InitializeAttributeDelegate();
}

void UGAS_WidgetComponent::InitAbilitySystemData()
{
	GASCharacter = Cast<AGAS_BaseCharacter>(GetOwner());
	AttributeSet = Cast<UGAS_AttributeSet>(GASCharacter->GetAttributeSet());
	AbilitySystemComponent = Cast<UGAS_AbilitySystemComponent>(GASCharacter->GetAbilitySystemComponent());
}

//检查这三个指针是否已经初始化
bool UGAS_WidgetComponent::IsASCInitialized() const
{
	return AbilitySystemComponent.IsValid() && AttributeSet.IsValid();
}

void UGAS_WidgetComponent::InitializeAttributeDelegate()
{
	//如果属性未初始化
	if (!AttributeSet->bAttributesInitialized)
	{
		//监听属性初始化的委托，绑定回调函数BindToAttributeChanges()
		AttributeSet->OnAttributesInitialized.AddDynamic(this,&ThisClass::BindToAttributeChanges);
	}
	else
	{
		//属性已初始化，直接调用BindToAttributeChanges函数(监听实际的游戏属性变化，以更新UI)
		BindToAttributeChanges();
	}
}

//响应ASC初始化
void UGAS_WidgetComponent::OnASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	//通过委托发送的AbilitySystemComponent和AttributeSet 获得GAS_AbilitySystemComponent和GAS_AttributeSet
	AbilitySystemComponent = Cast<UGAS_AbilitySystemComponent>(ASC);
	AttributeSet = Cast<UGAS_AttributeSet>(AS);
	
	//再检查指针是否已经初始化完成
	if (!IsASCInitialized()) return;
	//检查属性集是否已通过首个游戏效果初始化
	//如果未初始化则绑定到某个委托，该委托会在初始化时触发
	InitializeAttributeDelegate();
}

//监听实际的游戏属性变化，以更新UI
void UGAS_WidgetComponent::BindToAttributeChanges()
{
	//遍历AttributeMap
	for (const TTuple<FGameplayAttribute,FGameplayAttribute>& Pair : AttributeMap)
	{
		BindWidgetToAttributeChanges(GetUserWidgetObject(),Pair);
		
		//遍历它包含的所有子小部件
		GetUserWidgetObject()->WidgetTree->ForEachWidget([this,&Pair](UWidget* ChildWidget)
		{
			BindWidgetToAttributeChanges(ChildWidget,Pair);
		});
	}
}

void UGAS_WidgetComponent::BindWidgetToAttributeChanges(UWidget* WidgetObject,const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const
{
	//检查该组件拥有的用户小部件对象，判断是否为GAS_AttributeWidget,如果是则处理，如果不是遍历拥有部件的所有子部件
	UGAS_AttributeWidget* AttributeWidget = Cast<UGAS_AttributeWidget>(WidgetObject);
	if (!IsValid(AttributeWidget)) return;
	//检查其属性对是否与我们在Widget里拥有属性对的匹配
	if (!AttributeWidget->MatchesAttribute(Pair)) return;
	AttributeWidget->AvatarActor = GASCharacter;
		
	//调用属性变化来更新该小部件
	AttributeWidget->OnAttributeChange(Pair,AttributeSet.Get(),0.f);//初始值
	//能力系统组件在属性变化时广播的一个委托
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Pair.Key).AddLambda([this,AttributeWidget,&Pair](const FOnAttributeChangeData& AttributeChangeData)
	{
		AttributeWidget->OnAttributeChange(Pair,AttributeSet.Get(),AttributeChangeData.OldValue);
	});
}

