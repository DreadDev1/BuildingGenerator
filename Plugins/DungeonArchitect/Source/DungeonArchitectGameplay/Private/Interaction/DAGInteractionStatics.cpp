//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Interaction/DAGInteractionStatics.h"

#include "Interaction/DAGInteractableTarget.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Actor.h"

UDAG_InteractionStatics::UDAG_InteractionStatics()
	: Super(FObjectInitializer::Get())
{
}

AActor* UDAG_InteractionStatics::GetActorFromDAInteractableTarget(TScriptInterface<IDAG_InteractableTarget> InteractableTarget) {
	if (UObject* Object = InteractableTarget.GetObject()) {
		if (AActor* Actor = Cast<AActor>(Object)) {
			return Actor;
		}
		else if (UActorComponent* ActorComponent = Cast<UActorComponent>(Object)) {
			return ActorComponent->GetOwner();
		}
		else {
			unimplemented();
		}
	}

	return nullptr;
}

void UDAG_InteractionStatics::GetDAInteractableTargetsFromActor(AActor* Actor, TArray<TScriptInterface<IDAG_InteractableTarget>>& OutInteractableTargets)
{
	// If the actor is directly interactable, return that.
	TScriptInterface<IDAG_InteractableTarget> InteractableActor(Actor);
	if (InteractableActor)
	{
		OutInteractableTargets.Add(InteractableActor);
	}

	// If the actor isn't interactable, it might have a component that has a interactable interface.
	TArray<UActorComponent*> InteractableComponents = Actor ? Actor->GetComponentsByInterface(UDAG_InteractableTarget::StaticClass()) : TArray<UActorComponent*>();
	for (UActorComponent* InteractableComponent : InteractableComponents)
	{
		OutInteractableTargets.Add(TScriptInterface<IDAG_InteractableTarget>(InteractableComponent));
	}
}

void UDAG_InteractionStatics::AppendDAInteractableTargetsFromOverlapResults(const TArray<FOverlapResult>& OverlapResults, TArray<TScriptInterface<IDAG_InteractableTarget>>& OutInteractableTargets)
{
	for (const FOverlapResult& Overlap : OverlapResults) {
		AActor* Actor = Overlap.GetActor();
		if (Actor && Actor->Implements<UDAG_InteractableTarget>()) {
			TScriptInterface<IDAG_InteractableTarget> InteractableActor(Actor);
			OutInteractableTargets.AddUnique(InteractableActor);
		}

		UPrimitiveComponent* Component = Overlap.GetComponent();
		if (Component && Component->Implements<UDAG_InteractableTarget>()) {
			TScriptInterface<IDAG_InteractableTarget> InteractableComponent(Component);
			OutInteractableTargets.AddUnique(InteractableComponent);
		}
	}
}

void UDAG_InteractionStatics::AppendDAInteractableTargetsFromHitResult(const FHitResult& HitResult, TArray<TScriptInterface<IDAG_InteractableTarget>>& OutInteractableTargets)
{
	AActor* Actor = HitResult.GetActor();
	if (Actor && Actor->Implements<UDAG_InteractableTarget>()) {
		TScriptInterface<IDAG_InteractableTarget> InteractableActor(Actor);
		OutInteractableTargets.AddUnique(InteractableActor);
	}

	UPrimitiveComponent* Component = HitResult.GetComponent();
	if (Component && Component->Implements<UDAG_InteractableTarget>()) {
		TScriptInterface<IDAG_InteractableTarget> InteractableComponent(Component);
		OutInteractableTargets.AddUnique(InteractableComponent);
	}
}

