//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Interaction/DAGInteractionOption.h"

#include "Components/SceneComponent.h"
#include "UObject/Interface.h"
#include "DAGInteractableTarget.generated.h"

struct FDAG_InteractionQuery;

class FDAG_InteractionOptionBuilder {
public:
	FDAG_InteractionOptionBuilder(TScriptInterface<IDAG_InteractableTarget> InterfaceTargetScope, TArray<FDAG_InteractionOption>& InteractOptions)
		: Scope(InterfaceTargetScope)
		, Options(InteractOptions)
	{
	}

	void AddInteractionOption(const FDAG_InteractionOption& Option)
	{
		FDAG_InteractionOption& OptionEntry = Options.Add_GetRef(Option);
		OptionEntry.InteractableTarget = Scope;
	}

private:
	TScriptInterface<IDAG_InteractableTarget> Scope;
	TArray<FDAG_InteractionOption>& Options;
};


UINTERFACE()
class UDAG_InteractableTarget : public UInterface
{
	GENERATED_BODY()
};

class IDAG_InteractableTarget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Dungeon Architect")
	void GatherInteractionOptions(const FDAG_InteractionQuery& InteractQuery, TArray<FDAG_InteractionOption>& Options);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Dungeon Architect")
	void Interact(AActor* InInstigator);
};


UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UDAG_InteractableTargetPivotPoint : public USceneComponent {
	GENERATED_BODY()
public:
};

