//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/DungeonForgeCommon.h"

#include "GameFramework/Actor.h"

FDungeonForgeActorTag::FDungeonForgeActorTag(const FGuid& InNodeGuid)
	: NodeGuid(InNodeGuid)
{
	Tag = CreateTag();
}

void FDungeonForgeActorTag::ApplyToActor(AActor* InActor) const {
	InActor->Tags.AddUnique(Tag);
}

bool FDungeonForgeActorTag::Contains(AActor* InActor) const {
	return InActor->Tags.Contains(Tag);
}

FName FDungeonForgeActorTag::CreateTag() const {
	return *FString("DUNGEON-FORGE-").Append(NodeGuid.ToString());
}

