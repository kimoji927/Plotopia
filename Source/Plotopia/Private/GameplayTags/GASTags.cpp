#include "GameplayTags/GASTags.h"

namespace GASTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(None,"GASTags.None","None");
	
	namespace SetByCaller
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Projectile,"GASTags.SetByCaller.Projectile","Tag for Set by Caller Magnitude for Projectiles.");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Melee,"GASTags.SetByCaller.Melee","Tag for Set by Caller Data Tag for Melee Attack.");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Secondary,"GASTags.SetByCaller.Secondary","Tag for Set by Caller Data Tag for Secondary Attack.");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Consume,"GASTags.SetByCaller.Consume","Tag for Set by Caller Magnitude for Consumable Items (e.g. health potion).");
	}
	
	namespace GASAbilities
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(BlockHitReact,"GASTags.GASAbilities.BlockHitReact","Tag for Blocking Hit React.")	
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ActivateOnGiven,"GASTags.GASAbilities.ActivateOnGiven","Tag for Abilities that should activate immediately once given.")	
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death,"GASTags.GASAbilities.Death","Tag for the Death Ability.")	
		
		namespace Player
		{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Primary,"GASTags.GASAbilities.Player.Primary","Tag for the Primary Ability")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Secondary,"GASTags.GASAbilities.Player.Secondary","Tag for the Secondary Ability")
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tertiary,"GASTags.GASAbilities.Player.Tertiary","Tag for the Tertiary Ability")
		}
		
		namespace Enemy
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact,"GASTags.GASAbilities.Enemy.HitReact","Tag for the Enemy HitReact Ability")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack,"GASTags.GASAbilities.Enemy.Attack","Tag for the Enemy Attack Ability")
		}
	}
	
	namespace GASEvents
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(KillScored,"GASTags.GASEvents.KillScored","Tag for the KillScored Event")
		
		namespace Player
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Primary,"GASTags.GASEvents.Player.Primary","Tag for the Primary Event")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Secondary,"GASTags.GASEvents.Player.Secondary","Tag for the Secondary Event")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact,"GASTags.GASEvents.Player.HitReact","Tag for the Player HitReact Event")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death,"GASTags.GASEvents.Player.Death","Tag for the Player Death Event")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConsumeItem,"GASTags.GASEvents.Player.ConsumeItem","Tag for the Player ConsumeItem Event (fired when an inventory consumable is used)")
		}
		
		namespace Enemy
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact,"GASTags.GASEvents.Enemy.HitReact","Tag for the Enemy HitReact Event")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(EndAttack,"GASTags.GASEvents.Enemy.EndAttack","Tag for the Enemy EndAttack Event")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(MeleeTraceHit,"GASTags.GASEvents.Enemy.MeleeTraceHit","Tag for the Enemy Melee Trace Hit")
		}
	}

	namespace Status
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dead,"GASTags.Status.Dead","Tag for the Dead Status")
	}
	
	namespace Cooldown
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Secondary,"GASTags.Cooldown.Secondary","Tag for the Secondary Cooldown");
	}
}
