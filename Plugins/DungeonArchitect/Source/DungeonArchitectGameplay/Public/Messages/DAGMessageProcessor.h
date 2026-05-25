//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Messages/MessageSystem/DAGMessageSubsystem.h"

#include "Components/ActorComponent.h"
#include "DAGMessageProcessor.generated.h"

namespace EEndPlayReason { enum Type : int; }

class UObject;

/**
 * UDAG_MessageProcessor
 * 
 * Base class for any message processor which observes other gameplay messages
 * and potentially re-emits updates (e.g., when a chain or combo is detected)
 * 
 * Note that these processors are spawned on the server once (not per player)
 * and should do their own internal filtering if only relevant for some players.
 */
UCLASS(BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class DUNGEONARCHITECTGAMEPLAY_API UDAG_MessageProcessor : public UActorComponent
{
	GENERATED_BODY()

public:
	//~UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of UActorComponent interface

	virtual void StartListening();
	virtual void StopListening();

protected:
	void AddListenerHandle(FDAG_MessageListenerHandle&& Handle);
	double GetServerTime() const;

private:
	TArray<FDAG_MessageListenerHandle> ListenerHandles;
};


