//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/DungeonModel.h"

#include "Core/Dungeon.h"

void UDungeonModel::Reset() {
	DungeonLayout = {};
	WorldMarkers.Reset();
}

FDungeonFloorSettings UDungeonModel::CreateFloorSettings(const UDungeonConfig* InConfig) const {
	return {};
}

void UDungeonModel::GenerateLayoutData(const UDungeonConfig* InConfig, FDungeonLayoutData& OutLayout) const {
	OutLayout = {};

	// Setup the dungeon transform.  The implementations can override this
	if (ADungeon* OuterDungeon = Cast<ADungeon>(GetOuter())) {
		OutLayout.DungeonTransform = OuterDungeon->GetActorTransform();
	}

	GenerateLayoutDataImpl(InConfig, OutLayout);
}

FBox UDungeonModel::GetDungeonBounds() const {
	return DungeonLayout.Bounds;
}

