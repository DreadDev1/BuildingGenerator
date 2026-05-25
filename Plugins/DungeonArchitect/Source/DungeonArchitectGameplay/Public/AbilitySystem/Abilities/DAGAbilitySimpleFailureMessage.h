//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "DAGAbilitySimpleFailureMessage.generated.h"

USTRUCT(BlueprintType)
struct FDAG_AbilitySimpleFailureMessage
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category="DAG")
	TObjectPtr<APlayerController> PlayerController = nullptr;

	UPROPERTY(BlueprintReadWrite, Category="DAG")
	FGameplayTagContainer FailureTags;

	UPROPERTY(BlueprintReadWrite, Category="DAG")
	FText UserFacingReason;
};

