//$ Copyright 2015-24, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "AbilitySystem/DAGGameplayEffectContext.h"

#include "AbilitySystem/DAGAbilitySourceInterface.h"

#include "Engine/HitResult.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#if UE_WITH_IRIS
#include "Iris/ReplicationState/PropertyNetSerializerInfoRegistry.h"
#include "Serialization/GameplayEffectContextNetSerializer.h"
#endif


#include UE_INLINE_GENERATED_CPP_BY_NAME(DAGGameplayEffectContext)

class FArchive;

FDAG_GameplayEffectContext* FDAG_GameplayEffectContext::ExtractEffectContext(struct FGameplayEffectContextHandle Handle)
{
	FGameplayEffectContext* BaseEffectContext = Handle.Get();
	if ((BaseEffectContext != nullptr) && BaseEffectContext->GetScriptStruct()->IsChildOf(FDAG_GameplayEffectContext::StaticStruct()))
	{
		return (FDAG_GameplayEffectContext*)BaseEffectContext;
	}

	return nullptr;
}

bool FDAG_GameplayEffectContext::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);

	// Not serialized for post-activation use:
	// CartridgeID

	return true;
}

#if UE_WITH_IRIS
namespace UE::Net
{
	// Forward to FGameplayEffectContextNetSerializer
	// Note: If FDAG_GameplayEffectContext::NetSerialize() is modified, a custom NetSerializesr must be implemented as the current fallback will no longer be sufficient.
	UE_NET_IMPLEMENT_FORWARDING_NETSERIALIZER_AND_REGISTRY_DELEGATES(DAG_GameplayEffectContext, FGameplayEffectContextNetSerializer);
}
#endif

void FDAG_GameplayEffectContext::SetAbilitySource(const IDAG_AbilitySourceInterface* InObject, float InSourceLevel)
{
	AbilitySourceObject = MakeWeakObjectPtr(Cast<const UObject>(InObject));
	//SourceLevel = InSourceLevel;
}

const IDAG_AbilitySourceInterface* FDAG_GameplayEffectContext::GetAbilitySource() const
{
	return Cast<IDAG_AbilitySourceInterface>(AbilitySourceObject.Get());
}

const UPhysicalMaterial* FDAG_GameplayEffectContext::GetPhysicalMaterial() const
{
	if (const FHitResult* HitResultPtr = GetHitResult())
	{
		return HitResultPtr->PhysMaterial.Get();
	}
	return nullptr;
}

