//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/Assets/Forge/DungeonForgeAssetTypeAction.h"

#include "Core/Common/ContentBrowserMenuExtensions.h"
#include "DungeonArchitectEditorModule.h"
#include "Frameworks/Forge/DungeonForgeAsset.h"

#define LOCTEXT_NAMESPACE "DungeonForgeAssetTypeActions"

//////////////////////// FDungeonForgeAssetTypeActions ////////////////////////
FText FDungeonForgeAssetTypeActions::GetName() const {
	return LOCTEXT("DungeonForgeAssetTypeActionsName", "Dungeon Forge");
}

FColor FDungeonForgeAssetTypeActions::GetTypeColor() const {
	return FColor::Purple;
}

UClass* FDungeonForgeAssetTypeActions::GetSupportedClass() const {
	return UDungeonForgeAsset::StaticClass();
}

void FDungeonForgeAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects,
													TSharedPtr<class IToolkitHost> EditWithinLevelEditor) {
	FAssetTypeActions_Base::OpenAssetEditor(InObjects, EditWithinLevelEditor);

	/*
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid()
										? EToolkitMode::WorldCentric
										: EToolkitMode::Standalone;
	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt) {
		if (UDungeonForgeAsset* FlowAsset = Cast<UDungeonForgeAsset>(*ObjIt)) {
			const TSharedRef<FCellFlowEditor> NewCellFlowEditor(new FCellFlowEditor());
			NewCellFlowEditor->InitEditor(Mode, EditWithinLevelEditor, FlowAsset);
		}
	}
	*/
}

uint32 FDungeonForgeAssetTypeActions::GetCategories() {
	return IDungeonArchitectEditorModule::Get().GetDungeonAssetCategoryBit();
}

void FDungeonForgeAssetTypeActions::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) {
}


#undef LOCTEXT_NAMESPACE

