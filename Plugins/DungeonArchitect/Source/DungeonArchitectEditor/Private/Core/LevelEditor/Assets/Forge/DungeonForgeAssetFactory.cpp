//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/Assets/Forge/DungeonForgeAssetFactory.h"

#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorUtils.h"
#include "Frameworks/Forge/DungeonForgeAsset.h"

//////////////////////////////////// UForgeAssetFactory ////////////////////////////////////
UDungeonForgeAssetFactory::UDungeonForgeAssetFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	SupportedClass = UDungeonForgeAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UDungeonForgeAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) {
	UDungeonForgeAsset* NewAsset = NewObject<UDungeonForgeAsset>(InParent, Class, Name, Flags | RF_Transactional);
	FDungeonForgeEditorUtils::InitializeAsset(NewAsset);
	NewAsset->Version = static_cast<int>(EDungeonForgeAssetVersion::LatestVersion);
	return NewAsset;
}

bool UDungeonForgeAssetFactory::CanCreateNew() const {
	return true;
}

