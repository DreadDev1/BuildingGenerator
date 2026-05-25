//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DungeonSpawnLogic.generated.h"

class ADungeon;
class UDungeonBuilder;
class UDungeonModel;
class UDungeonConfig;
class UDungeonQuery;

/**
*
*/
UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable, HideDropDown)
class DUNGEONARCHITECTRUNTIME_API UDungeonSpawnLogic : public UObject {
    GENERATED_BODY()

public:
    /** Called by the theming engine when a dungeon item is spawned into the scene */
    UFUNCTION(BlueprintNativeEvent, Category = "Dungeon")
    void OnItemSpawn(AActor* Actor, const ADungeon* Dungeon);
    virtual void OnItemSpawn_Implementation(AActor* Actor, const ADungeon* Dungeon);

    void SetExecState(UWorld* InWorld, const FRandomStream* InRandom);
    void ClearExecState();
    
    // UObject interface.
    virtual UWorld* GetWorld() const override;
#if WITH_EDITOR
    virtual bool ImplementsGetWorld() const override { return true; }
#endif  // WITH_EDITOR

    UFUNCTION(BlueprintCallable, Category="Dungeon")
    float GetRandomFloat();
    
    UFUNCTION(BlueprintCallable, Category="Dungeon")
    int GetRandomInt();
    
private:
    /** Cached world. */
    TWeakObjectPtr<UWorld> WeakCachedWorld;
    const FRandomStream* CachedRandom{};
};

