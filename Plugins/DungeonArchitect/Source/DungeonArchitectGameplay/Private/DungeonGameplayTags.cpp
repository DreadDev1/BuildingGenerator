//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "DungeonGameplayTags.h"


namespace DAG_Tags {
	UE_DEFINE_GAMEPLAY_TAG(Gameplay_Damage, "DAG.Damage");
	UE_DEFINE_GAMEPLAY_TAG(Gameplay_DamageImmunity, "DAG.DamageImmunity");
	UE_DEFINE_GAMEPLAY_TAG(Gameplay_DamageSelfDestruct, "DAG.Damage.SelfDestruct");
	UE_DEFINE_GAMEPLAY_TAG(Gameplay_FellOutOfWorld, "DAG.Damage.FellOutOfWorld");
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_Death, "DAG.GameplayEvent.Death", "Event that fires on death. This event only fires on the server.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_Reset, "DAG.GameplayEvent.Reset", "Event that fires once a player reset is executed.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_RequestReset, "DAG.GameplayEvent.RequestReset", "Event to request a player's pawn to be instantly replaced with a new one at a valid spawn location.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActivateFail_IsDead, "DAG.Ability.ActivateFail.IsDead", "Ability failed to activate because its owner is dead.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActivateFail_Cooldown, "DAG.Ability.ActivateFail.Cooldown", "Ability failed to activate because it is on cool down.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActivateFail_Cost, "DAG.Ability.ActivateFail.Cost", "Ability failed to activate because it did not pass the cost checks.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActivateFail_TagsBlocked, "DAG.Ability.ActivateFail.TagsBlocked", "Ability failed to activate because tags are blocking it.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActivateFail_TagsMissing, "DAG.Ability.ActivateFail.TagsMissing", "Ability failed to activate because tags are missing.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActivateFail_Networking, "DAG.Ability.ActivateFail.Networking", "Ability failed to activate because it did not pass the network checks.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_ActivateFail_ActivationGroup, "DAG.Ability.ActivateFail.ActivationGroup", "Ability failed to activate because of its activation group.");
	UE_DEFINE_GAMEPLAY_TAG(Ability_SimpleFailureMessage, "DAG.Ability.UserFacingSimpleActivateFail.Message");
	UE_DEFINE_GAMEPLAY_TAG(Ability_PlayMontageFailureMessage, "DAG.Ability.PlayMontageOnActivateFail.Message");
	
	UE_DEFINE_GAMEPLAY_TAG(AddNotification_Message, "DAG.AddNotification.Message");
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cheat_GodMode, "DAG.Cheat.GodMode", "GodMode cheat is active on the owner.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cheat_UnlimitedHealth, "DAG.Cheat.UnlimitedHealth", "UnlimitedHealth cheat is active on the owner.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Crouching, "DAG.Status.Crouching", "Target is crouching.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_AutoRunning, "DAG.Status.AutoRunning", "Target is auto-running.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Death, "DAG.Status.Death", "Target has the death status.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Death_Dying, "DAG.Status.Death.Dying", "Target has begun the death process.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Death_Dead, "DAG.Status.Death.Dead", "Target has finished the death process.");
	
	UE_DEFINE_GAMEPLAY_TAG(Elimination_Message, "DAG.Elimination.Message");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Message, "DAG.Damage.Message");
	
	UE_DEFINE_GAMEPLAY_TAG(AbilityInputBlocked, "DAG.Gameplay.AbilityInputBlocked");

	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Dash, "DAG.InputTag.Ability.Dash");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Heal, "DAG.InputTag.Ability.Heal");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Melee, "DAG.InputTag.Ability.Melee");
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Drop, "DAG.InputTag.Ability.Quickslot.Drop");
	UE_DEFINE_GAMEPLAY_TAG(Input_Jump, "DAG.InputTag.Jump");
	UE_DEFINE_GAMEPLAY_TAG(Input_Weapon_ADS, "DAG.InputTag.Weapon.ADS");
	UE_DEFINE_GAMEPLAY_TAG(Input_Weapon_Fire, "DAG.InputTag.Weapon.Fire");
	UE_DEFINE_GAMEPLAY_TAG(Input_Weapon_FireAuto, "DAG.InputTag.Weapon.FireAuto");
	UE_DEFINE_GAMEPLAY_TAG(Input_Weapon_Grenade, "DAG.InputTag.Weapon.Grenade");
	UE_DEFINE_GAMEPLAY_TAG(Input_Weapon_Reload, "DAG.InputTag.Weapon.Reload");
	
};



