//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/Grid/Volumes/GridDungeonPlatformVolume.h"

#include "Components/BrushComponent.h"
#include "Engine/CollisionProfile.h"

AGridDungeonPlatformVolume::AGridDungeonPlatformVolume(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , CellType(EGridDungeonCellType::Corridor)
{
    if (UBrushComponent* BrushComp = GetBrushComponent()) {
        BrushComp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
        BrushComp->SetGenerateOverlapEvents(false);
    }
}

