//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Engine/CancellableAsyncAction.h"
#include "UObject/ScriptInterface.h"
#include "UObject/WeakInterfacePtr.h"
#include "DAGAsyncAction_ObserveTeam.generated.h"

class IDAG_TeamAgentInterface;
struct FFrame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDAG_TeamObservedAsyncDelegate, bool, bTeamSet, int32, TeamId);

/**
 * Watches for team changes in the specified object
 */
UCLASS()
class UDAG_AsyncAction_ObserveTeam : public UCancellableAsyncAction
{
	GENERATED_UCLASS_BODY()

public:
	// Watches for team changes on the specified team agent
	//  - It will will fire once immediately to give the current team assignment
	//  - For anything that can ever belong to a team (implements IDAG_TeamAgentInterface),
	//    it will also listen for team assignment changes in the future
	UFUNCTION(BlueprintCallable, Category="DAG", meta=(BlueprintInternalUseOnly="true", Keywords="Watch"))
	static UDAG_AsyncAction_ObserveTeam* ObserveTeam(UObject* TeamAgent);

	//~UBlueprintAsyncActionBase interface
	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;
	//~End of UBlueprintAsyncActionBase interface

public:
	// Called when the team is set or changed
	UPROPERTY(BlueprintAssignable, Category="DAG")
	FDAG_TeamObservedAsyncDelegate OnTeamChanged;

private:
	// Watches for team changes on the specified team actor
	static UDAG_AsyncAction_ObserveTeam* InternalObserveTeamChanges(TScriptInterface<IDAG_TeamAgentInterface> TeamActor);

private:
	UFUNCTION()
	void OnWatchedAgentChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

	TWeakInterfacePtr<IDAG_TeamAgentInterface> TeamInterfacePtr;
};

