//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/Assets/CellFlow/CellFlowAssetTypeAction.h"

#include "Builders/CellFlow/CellFlowAsset.h"
#include "Builders/CellFlow/CellFlowConfig.h"
#include "Core/Common/ContentBrowserMenuExtensions.h"
#include "Core/Editors/FlowEditor/BaseEditors/CellFlowEditor.h"
#include "DungeonArchitectEditorModule.h"

#define LOCTEXT_NAMESPACE "CellFlowFlowAssetTypeActions"

//////////////////////// FCellFlowAssetTypeActions ////////////////////////
FText FCellFlowAssetTypeActions::GetName() const {
	return LOCTEXT("CellFlowAssetTypeActionsName", "Cell Flow Graph");
}

FColor FCellFlowAssetTypeActions::GetTypeColor() const {
	return FColor::Purple;
}

UClass* FCellFlowAssetTypeActions::GetSupportedClass() const {
	return UCellFlowAsset::StaticClass();
}

void FCellFlowAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects,
													TSharedPtr<class IToolkitHost> EditWithinLevelEditor) {
	FAssetTypeActions_Base::OpenAssetEditor(InObjects, EditWithinLevelEditor);

	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid()
										? EToolkitMode::WorldCentric
										: EToolkitMode::Standalone;
	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt) {
		if (UCellFlowAsset* FlowAsset = Cast<UCellFlowAsset>(*ObjIt)) {
			const TSharedRef<FCellFlowEditor> NewCellFlowEditor(new FCellFlowEditor());
			NewCellFlowEditor->InitEditor(Mode, EditWithinLevelEditor, FlowAsset);
		}
	}
}

uint32 FCellFlowAssetTypeActions::GetCategories() {
	return IDungeonArchitectEditorModule::Get().GetDungeonAssetCategoryBit();
}

const TArray<FText>& FCellFlowAssetTypeActions::GetSubMenus() const {
	static const TArray<FText> SubMenus = {
		FDAContentBrowserSubMenuNames::CellFlow
	};
	return SubMenus;
}

void FCellFlowAssetTypeActions::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) {
}


//////////////////////// FCellFlowConfigMarkerSettingsTypeActions ////////////////////////
FText FCellFlowConfigMarkerSettingsTypeActions::GetName() const {
	return LOCTEXT("CellFlowMarkerSettingAssetTypeActionsName", "Cell Flow Marker Settings");
}

FColor FCellFlowConfigMarkerSettingsTypeActions::GetTypeColor() const {
	return FColor::Turquoise;
}

UClass* FCellFlowConfigMarkerSettingsTypeActions::GetSupportedClass() const {
	return UCellFlowConfigMarkerSettings::StaticClass();
}

uint32 FCellFlowConfigMarkerSettingsTypeActions::GetCategories() {
	return IDungeonArchitectEditorModule::Get().GetDungeonAssetCategoryBit();
}

const TArray<FText>& FCellFlowConfigMarkerSettingsTypeActions::GetSubMenus() const {
	static const TArray<FText> SubMenus = { FDAContentBrowserSubMenuNames::CellFlow };
	return SubMenus;
}


#undef LOCTEXT_NAMESPACE


