//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DAGVerbMessageHelpers.generated.h"

struct FGameplayCueParameters;
struct FDAG_VerbMessage;

class APlayerController;
class APlayerState;
class UObject;
struct FFrame;


UCLASS()
class DUNGEONARCHITECTGAMEPLAY_API UDAG_VerbMessageHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DAG")
	static APlayerState* DAG_GetPlayerStateFromObject(UObject* Object);

	UFUNCTION(BlueprintCallable, Category = "DAG")
	static APlayerController* DAG_GetPlayerControllerFromObject(UObject* Object);

	UFUNCTION(BlueprintCallable, Category = "DAG")
	static FGameplayCueParameters DAG_VerbMessageToCueParameters(const FDAG_VerbMessage& Message);

	UFUNCTION(BlueprintCallable, Category = "DAG")
	static FDAG_VerbMessage DAG_CueParametersToVerbMessage(const FGameplayCueParameters& Params);
};

