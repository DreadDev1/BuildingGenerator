//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DungeonStreamingChunkEvent.generated.h"

class ADungeon;

UINTERFACE(Blueprintable)
class DUNGEONARCHITECTRUNTIME_API UDungeonStreamingChunkEventInterface : public UInterface
{
	GENERATED_BODY()
};

class DUNGEONARCHITECTRUNTIME_API IDungeonStreamingChunkEventInterface
{    
	GENERATED_BODY()

public:
	/** Fired when the chunk is made visible, e.g. when the player gets close */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, CallInEditor, Category="Dungeon Streaming Chunks")
	void OnChunkVisible(ADungeon* OwningDungeon);

	/** Fired when the chunk is hidden, e.g. when the player gets far away. It might still be kept in memory */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, CallInEditor, Category="Dungeon Streaming Chunks")
	void OnChunkHidden(ADungeon* OwningDungeon);

	/** When the chunk was loaded for the first time. Do you one-time initializations here */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, CallInEditor, Category="Dungeon Streaming Chunks")
	void OnChunkLoaded(ADungeon* OwningDungeon);

	/** When the chunk is unloaded from memory */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, CallInEditor, Category="Dungeon Streaming Chunks")
	void OnChunkUnloaded(ADungeon* OwningDungeon);
};

