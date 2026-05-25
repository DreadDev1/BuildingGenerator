//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Layout/DungeonLayoutData.h"
#include "Core/Markers/DungeonMarker.h"
#include "Core/Utils/DungeonMeshGeometry.h"
#include "DungeonModel.generated.h"

class UDungeonConfig;
class UDungeonSpawnLogic;
struct FDungeonLayoutData;

UCLASS(Blueprintable)
class DUNGEONARCHITECTRUNTIME_API UDungeonModel : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category="Dungeon")
    FDungeonLayoutData DungeonLayout;
    
    // Final marker (point) list after the theme engine and PCG runs
    UPROPERTY()
    TArray<FDungeonMarkerInstance> WorldMarkers;
    
    // The initial marker list emitted from the layout generator
    UPROPERTY()
    TArray<FDungeonMarkerInstance> CachedBaseLayoutMarkers;

public:
    virtual void Reset();
    virtual FDungeonFloorSettings CreateFloorSettings(const UDungeonConfig* InConfig) const;

    void GenerateLayoutData(const UDungeonConfig* InConfig, FDungeonLayoutData& OutLayout) const;
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Dungeon")
    FBox GetDungeonBounds() const;

protected:
    virtual void GenerateLayoutDataImpl(const UDungeonConfig* InConfig, FDungeonLayoutData& OutLayout) const {}
};

