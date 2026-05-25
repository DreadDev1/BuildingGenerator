//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Messages/DAGVerbMessageReplication.h"

#include "Messages/DAGVerbMessage.h"
#include "Messages/MessageSystem/DAGMessageSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DAGVerbMessageReplication)

//////////////////////////////////////////////////////////////////////
// FDAG_VerbMessageReplicationEntry

FString FDAG_VerbMessageReplicationEntry::GetDebugString() const
{
	return Message.ToString();
}

//////////////////////////////////////////////////////////////////////
// FDAG_VerbMessageReplication

void FDAG_VerbMessageReplication::AddMessage(const FDAG_VerbMessage& Message)
{
	FDAG_VerbMessageReplicationEntry& NewStack = CurrentMessages.Emplace_GetRef(Message);
	MarkItemDirty(NewStack);
}

void FDAG_VerbMessageReplication::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	// 	for (int32 Index : RemovedIndices)
	// 	{
	// 		const FGameplayTag Tag = CurrentMessages[Index].Tag;
	// 		TagToCountMap.Remove(Tag);
	// 	}
}

void FDAG_VerbMessageReplication::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		const FDAG_VerbMessageReplicationEntry& Entry = CurrentMessages[Index];
		RebroadcastMessage(Entry.Message);
	}
}

void FDAG_VerbMessageReplication::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		const FDAG_VerbMessageReplicationEntry& Entry = CurrentMessages[Index];
		RebroadcastMessage(Entry.Message);
	}
}

void FDAG_VerbMessageReplication::RebroadcastMessage(const FDAG_VerbMessage& Message)
{
	check(Owner);
	UDAG_MessageSubsystem& MessageSystem = UDAG_MessageSubsystem::Get(Owner);
	MessageSystem.BroadcastMessage(Message.Verb, Message);
}

