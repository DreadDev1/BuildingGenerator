//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Builders/Grid/GridDungeonModel.h"
#include "Core/Volumes/DungeonVolume.h"
#include "GridDungeonPlatformVolume.generated.h"

/**
*
*/
UCLASS(Blueprintable)
class DUNGEONARCHITECTRUNTIME_API AGridDungeonPlatformVolume : public ADungeonVolume {
    GENERATED_BODY()

public:
    AGridDungeonPlatformVolume(const FObjectInitializer& ObjectInitializer);

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    EGridDungeonCellType CellType; // TODO: abstract this

    /**
     * Forces the platform to be generated in the volume's height.  Enabling this might not create a valid dungeon,
     * as some volume placements might lead to impossible height differences
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    bool bUseVolumeHeight = false;
};

