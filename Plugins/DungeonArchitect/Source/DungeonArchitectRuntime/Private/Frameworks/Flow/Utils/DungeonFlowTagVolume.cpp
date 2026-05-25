//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/Utils/DungeonFlowTagVolume.h"

#include "Components/BrushComponent.h"
#include "EngineUtils.h"

ADungeonFlowTagVolume::ADungeonFlowTagVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UBrushComponent* BrushComp = GetBrushComponent();
	if (BrushComp) {
		BrushComp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		BrushComp->SetGenerateOverlapEvents(false);
	}
}

void ADungeonFlowTagVolume::FindInWorld(const ADungeon* InDungeon, TArray<TWeakObjectPtr<ADungeonFlowTagVolume>>& OutVolumes) {
	if (IsValid(InDungeon)) {
		for (TActorIterator<ADungeonFlowTagVolume> It(InDungeon->GetWorld()); It; ++It) {
			if (ADungeonFlowTagVolume* Volume = *It) {
				if (Volume->Dungeon == InDungeon) {
					OutVolumes.Add(Volume);
				}
			}
		}
	}
}

