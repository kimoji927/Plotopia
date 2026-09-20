#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace GASTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(None);
	
	namespace SetByCaller
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Projectile);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Melee);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Secondary);
		/** 消耗品使用效果（回血/回蓝等）的 SetByCaller 数值标签 */
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Consume);
	}
	
	namespace GASAbilities
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(BlockHitReact);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ActivateOnGiven);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death);
		
		namespace Player
		{
			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Primary);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Secondary);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tertiary);
		}

		namespace Enemy
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack);
		}
		
	}

	namespace GASEvents
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(KillScored);
		
		namespace Player
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Primary);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Secondary);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death);
			/** 背包消耗品被成功使用的事件标签 */
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConsumeItem);
		}

		namespace Enemy
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(EndAttack);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(MeleeTraceHit);
		}
		
	}

	namespace Status
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dead);
	}
	
	namespace Cooldown
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Secondary);
	}
}
