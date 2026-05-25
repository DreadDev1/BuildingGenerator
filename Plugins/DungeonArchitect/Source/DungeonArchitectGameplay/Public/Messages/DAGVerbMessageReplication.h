//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Messages/DAGVerbMessage.h"

#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "DAGVerbMessageReplication.generated.h"

class UObject;
struct FDAG_VerbMessageReplication;
struct FNetDeltaSerializeInfo;

/**
 * Represents one verb message
 */
USTRUCT(BlueprintType)
struct FDAG_VerbMessageReplicationEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FDAG_VerbMessageReplicationEntry()
	{}

	FDAG_VerbMessageReplicationEntry(const FDAG_VerbMessage& InMessage)
		: Message(InMessage)
	{
	}

	FString GetDebugString() const;

private:
	friend FDAG_VerbMessageReplication;

	UPROPERTY()
	FDAG_VerbMessage Message;
};

/** Container of verb messages to replicate */
USTRUCT(BlueprintType)
struct FDAG_VerbMessageReplication : public FFastArraySerializer
{
	GENERATED_BODY()

	FDAG_VerbMessageReplication()
	{
	}

public:
	void SetOwner(UObject* InOwner) { Owner = InOwner; }

	// Broadcasts a message from server to clients
	void AddMessage(const FDAG_VerbMessage& Message);

	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FDAG_VerbMessageReplicationEntry, FDAG_VerbMessageReplication>(CurrentMessages, DeltaParms, *this);
	}

private:
	void RebroadcastMessage(const FDAG_VerbMessage& Message);

private:
	// Replicated list of gameplay tag stacks
	UPROPERTY()
	TArray<FDAG_VerbMessageReplicationEntry> CurrentMessages;
	
	// Owner (for a route to a world)
	UPROPERTY()
	TObjectPtr<UObject> Owner = nullptr;
};

template<>
struct TStructOpsTypeTraits<FDAG_VerbMessageReplication> : public TStructOpsTypeTraitsBase2<FDAG_VerbMessageReplication>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

