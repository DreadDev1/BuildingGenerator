//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/SceneProviders/Utils/DungeonSceneProviderLib.h"

#include "Core/Actors/DungeonInstancedMeshActor.h"

/////////////////////////////// FDungeonSceneProviderLib ///////////////////////////////
const FGuid FDungeonSceneProviderLib::StaticInstancedMeshNodeId = FGuid("27A91105-8A9B-4D9A-A947-70C47D5C97A6"); 

bool FDungeonSceneProviderLib::GetNodeId(AActor* InActor, const FName& InDungeonTag, FGuid& OutNodeId) {
	if (!IsValid(InActor)) return false;
	if (!InActor->IsValidLowLevel()) return false;
	if (!InActor->ActorHasTag(InDungeonTag)) return false;
	if (!IsValid(InActor)) return false;

	if (InActor->IsA<ADungeonInstancedMeshActor>()) {
		OutNodeId = FGuid();
		return true;
	}
    
	// Find the Node tag
	for (const FName& Tag : InActor->Tags) {
		FString TagString = Tag.ToString();
		if (TagString.StartsWith("NODE-")) {
			FString NodeIdStr = TagString.RightChop(5);
			return FGuid::Parse(NodeIdStr, OutNodeId);
		}
	}
	return false;
}

