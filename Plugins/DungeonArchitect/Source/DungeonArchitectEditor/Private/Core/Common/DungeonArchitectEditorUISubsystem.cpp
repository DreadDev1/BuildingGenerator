//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Common/DungeonArchitectEditorUISubsystem.h"

#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorModeToolkit.h"

#include "Framework/Docking/LayoutExtender.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"

void UDungeonArchitectEditorUISubsystem::Initialize(FSubsystemCollectionBase& Collection) {
	Super::Initialize(Collection);
	
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.OnRegisterLayoutExtensions().AddUObject(this, &UDungeonArchitectEditorUISubsystem::RegisterLayoutExtensions);
}

void UDungeonArchitectEditorUISubsystem::Deinitialize() {
	Super::Deinitialize();
	
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.OnRegisterLayoutExtensions().RemoveAll(this);
}

void UDungeonArchitectEditorUISubsystem::RegisterLayoutExtensions(FLayoutExtender& Extender) const {
	Extender.ExtendLayout(LevelEditorTabIds::PlacementBrowser, ELayoutExtensionPosition::Below,
		FTabManager::FTab(FDungeonForgeTabsID::DetailsView, ETabState::OpenedTab));
}

