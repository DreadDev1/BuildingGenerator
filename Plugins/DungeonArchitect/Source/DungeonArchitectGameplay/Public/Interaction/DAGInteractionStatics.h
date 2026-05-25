//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Interaction/DAGInteractableTarget.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ScriptInterface.h"
#include "DAGInteractionStatics.generated.h"

struct FOverlapResult;
class AActor;
struct FHitResult;

UCLASS()
class UDAG_InteractionStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UDAG_InteractionStatics();

public:
	UFUNCTION(BlueprintCallable, Category="Dungeon Architect")
	static AActor* GetActorFromDAInteractableTarget(TScriptInterface<IDAG_InteractableTarget> InteractableTarget);

	UFUNCTION(BlueprintCallable, Category="Dungeon Architect")
	static void GetDAInteractableTargetsFromActor(AActor* Actor, TArray<TScriptInterface<IDAG_InteractableTarget>>& OutInteractableTargets);

	static void AppendDAInteractableTargetsFromOverlapResults(const TArray<FOverlapResult>& OverlapResults, TArray<TScriptInterface<IDAG_InteractableTarget>>& OutInteractableTargets);
	static void AppendDAInteractableTargetsFromHitResult(const FHitResult& HitResult, TArray<TScriptInterface<IDAG_InteractableTarget>>& OutInteractableTargets);
};

