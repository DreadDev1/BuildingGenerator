//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/Assets/CellFlow/CellFlowAssetFactory.h"

#include "Builders/CellFlow/CellFlowAsset.h"
#include "Builders/CellFlow/CellFlowConfig.h"
#include "Core/Editors/FlowEditor/FlowEditorUtils.h"

//////////////////////////////////// UCellFlowAssetFactory ////////////////////////////////////
UCellFlowAssetFactory::UCellFlowAssetFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	SupportedClass = UCellFlowAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UCellFlowAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) {
	UCellFlowAsset* NewAsset = NewObject<UCellFlowAsset>(InParent, Class, Name, Flags | RF_Transactional);
	FFlowEditorUtils::InitializeFlowAsset(NewAsset);
	NewAsset->Version = static_cast<int>(ECellFlowAssetVersion::LatestVersion);
	return NewAsset;
}

bool UCellFlowAssetFactory::CanCreateNew() const {
	return true;
}

//////////////////////////////////// UCellFlowConfigMarkerSettingsFactory ////////////////////////////////////
UCellFlowConfigMarkerSettingsFactory::UCellFlowConfigMarkerSettingsFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	SupportedClass = UCellFlowConfigMarkerSettings::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UCellFlowConfigMarkerSettingsFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) {
	UCellFlowConfigMarkerSettings* NewAsset = NewObject<UCellFlowConfigMarkerSettings>(InParent, Class, Name, Flags | RF_Transactional);
	return NewAsset;
}

bool UCellFlowConfigMarkerSettingsFactory::CanCreateNew() const {
	return true;
}

