//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DAGInteractionOption.generated.h"

class IDAG_InteractableTarget;
class UUserWidget;

USTRUCT(BlueprintType)
struct FDAG_InteractionOption {
	GENERATED_BODY()
	
public:
	/** The interactable target */
	UPROPERTY(BlueprintReadWrite, Category="Dungeon Architect")
	TScriptInterface<IDAG_InteractableTarget> InteractableTarget;

	/** Simple text the interaction might return */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Architect")
	FText Text;

	/** Simple sub-text the interaction might return */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Architect")
	FText SubText;
	
	/** The widget to show for this kind of interaction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Architect")
	TSoftClassPtr<UUserWidget> InteractionWidgetClass;

	
public:
	FORCEINLINE bool operator==(const FDAG_InteractionOption& Other) const
	{
		return InteractableTarget == Other.InteractableTarget &&
			InteractionWidgetClass == Other.InteractionWidgetClass &&
			Text.IdenticalTo(Other.Text) &&
			SubText.IdenticalTo(Other.SubText);
	}

	FORCEINLINE bool operator!=(const FDAG_InteractionOption& Other) const
	{
		return !operator==(Other);
	}

	FORCEINLINE bool operator<(const FDAG_InteractionOption& Other) const
	{
		return InteractableTarget.GetInterface() < Other.InteractableTarget.GetInterface();
	}
};

