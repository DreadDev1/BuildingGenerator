//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Volumes/DungeonNegationVolume.h"

#include "Components/BrushComponent.h"
#include "Engine/CollisionProfile.h"
#include "UObject/UObjectIterator.h"

ADungeonNegationVolume::ADungeonNegationVolume(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer), Reversed(false), AffectsUserDefinedCells(true) {
    UBrushComponent* BrushComp = GetBrushComponent();
    if (BrushComp) {
        BrushComp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
        BrushComp->SetGenerateOverlapEvents(false);
    }
}

TArray<FDungeonNegationVolumeState> FDungeonNegationVolumeState::PopulateNegationVolumeBounds(const ADungeon* InDungeon) {
    TArray<FDungeonNegationVolumeState> NegationVolumes;
    UWorld* World = InDungeon ? InDungeon->GetWorld() : nullptr;
    if (!World) return {};
    FTransform DungeonInverseTransform = InDungeon ? InDungeon->GetActorTransform().Inverse() : FTransform::Identity;

    // Grab the bounds of all the negation volumes
    for (TObjectIterator<ADungeonNegationVolume> NegationVolume; NegationVolume; ++NegationVolume) {
        if (!NegationVolume->IsValidLowLevel() || !IsValid(*NegationVolume)) {
            continue;
        }
        if (NegationVolume->Dungeon != InDungeon) {
            continue;
        }

        FVector Origin, Extent;
        NegationVolume->GetActorBounds(false, Origin, Extent);
        FBox NegationBounds = FBox(Origin - Extent, Origin + Extent).TransformBy(DungeonInverseTransform);

        FDungeonNegationVolumeState State;
        State.Bounds = NegationBounds;
        State.bInverse = NegationVolume->Reversed;

        NegationVolumes.Add(State);
    }
    return NegationVolumes;
}

