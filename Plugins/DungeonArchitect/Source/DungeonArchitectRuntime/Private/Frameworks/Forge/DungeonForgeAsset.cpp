//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/DungeonForgeAsset.h"

#include "Frameworks/Forge/Graph/DungeonForgeGraph.h"

UDungeonForgeAsset::UDungeonForgeAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Version = static_cast<int32>(EDungeonForgeAssetVersion::LatestVersion);
	ForgeGraph = ObjectInitializer.CreateDefaultSubobject<UDungeonForgeGraph>(this, TEXT("ForgeGraph"));
}

