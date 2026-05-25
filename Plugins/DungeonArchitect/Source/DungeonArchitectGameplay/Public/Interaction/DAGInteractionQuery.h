//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DAGInteractionQuery.generated.h"

class AActor;
class AController;

USTRUCT(BlueprintType)
struct FDAG_InteractionQuery
{
	GENERATED_BODY()

public:
	/** The requesting pawn. */
	UPROPERTY(BlueprintReadWrite, Category="Dungeon Architect")
	TWeakObjectPtr<AActor> RequestingAvatar;

	/** Allow us to specify a controller - does not need to match the owner of the requesting avatar. */
	UPROPERTY(BlueprintReadWrite, Category="Dungeon Architect")
	TWeakObjectPtr<AController> RequestingController;

	/** A generic UObject to shove in extra data required for the interaction */
	UPROPERTY(BlueprintReadWrite, Category="Dungeon Architect")
	TWeakObjectPtr<UObject> OptionalObjectData;
};

