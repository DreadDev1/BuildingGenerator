//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "AbilitySystem/Attributes/DAGCombatAttributeSet.h"

#include "Net/UnrealNetwork.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(DAGCombatAttributeSet)

class FLifetimeProperty;


UDAG_CombatAttributeSet::UDAG_CombatAttributeSet()
	: BaseDamage(0.0f)
	, BaseHeal(0.0f)
{
}

void UDAG_CombatAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UDAG_CombatAttributeSet, BaseDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UDAG_CombatAttributeSet, BaseHeal, COND_OwnerOnly, REPNOTIFY_Always);
}

void UDAG_CombatAttributeSet::OnRep_BaseDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDAG_CombatAttributeSet, BaseDamage, OldValue);
}

void UDAG_CombatAttributeSet::OnRep_BaseHeal(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDAG_CombatAttributeSet, BaseHeal, OldValue);
}


