//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/DAGAttributeSetBase.h"

#include "AbilitySystemComponent.h"
#include "DAGCombatAttributeSet.generated.h"

class UObject;
struct FFrame;


/**
 * UDAG_CombatAttributeSet
 *
 *  Class that defines attributes that are necessary for applying damage or healing.
 *	Attribute examples include: damage, healing, attack power, and shield penetrations.
 */
UCLASS(BlueprintType)
class UDAG_CombatAttributeSet : public UDAG_AttributeSetBase
{
	GENERATED_BODY()

public:

	UDAG_CombatAttributeSet();

	ATTRIBUTE_ACCESSORS(UDAG_CombatAttributeSet, BaseDamage);
	ATTRIBUTE_ACCESSORS(UDAG_CombatAttributeSet, BaseHeal);

protected:

	UFUNCTION()
	void OnRep_BaseDamage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_BaseHeal(const FGameplayAttributeData& OldValue);

private:

	// The base amount of damage to apply in the damage execution.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseDamage, Category = "DAG|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData BaseDamage;

	// The base amount of healing to apply in the heal execution.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseHeal, Category = "DAG|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData BaseHeal;
};

