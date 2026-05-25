//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "AbilitySystem/Attributes/DAGAttributeSetBase.h"

#include "AbilitySystem/DAGAbilitySystemComponent.h"

UWorld* UDAG_AttributeSetBase::GetWorld() const {
	const UObject* Outer = GetOuter();
	check(Outer);
	return Outer->GetWorld();
}


UDAG_AbilitySystemComponent* UDAG_AttributeSetBase::GetDAGAbilitySystemComponent() const
{
	return Cast<UDAG_AbilitySystemComponent>(GetOwningAbilitySystemComponent());
}

