//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/FlowEditor/BaseEditors/CellFlowEditor.h"

#include "Builders/CellFlow/CellFlowAsset.h"
#include "Builders/CellFlow/CellFlowBuilder.h"
#include "Builders/CellFlow/CellFlowConfig.h"
#include "Core/Dungeon.h"
#include "Core/Editors/FlowEditor/DomainEditors/Impl/CellFlow/Viewport3D/CellFlowDomainEdLayoutGraph3D.h"
#include "Core/Editors/FlowEditor/FlowEditorSettings.h"
#include "Frameworks/Flow/Domains/FlowDomain.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutGraphDomain.h"

#define LOCTEXT_NAMESPACE "CellFlowEditor"

void FCellFlowEditor::InitDungeonConfig(UDungeonConfig* Config) {
	if (UCellFlowConfig* CellFlowConfig = Cast<UCellFlowConfig>(Config)) {
		CellFlowConfig->ParameterOverrides = EditorSettings->ParameterOverrides;
	}
}

void FCellFlowEditor::CreateDomainEditorsImpl() {
	// Register the domain editors
	DomainEditors.Add(MakeShareable(new FCellFlowDomainEdLayoutGraph3D));
}

FName FCellFlowEditor::GetFlowEdAppName() const {
	static const FName AppName = FName(TEXT("CellFlowEditor"));
	return AppName;
}

ADungeon* FCellFlowEditor::CreatePreviewDungeon(UWorld* World) {
	ADungeon* PreviewActor = World->SpawnActor<ADungeon>(FVector::ZeroVector, FQuat::Identity.Rotator());
	if (PreviewActor) {
		PreviewActor->SetBuilderClass(UCellFlowBuilder::StaticClass());
		if (UCellFlowConfig* CellFlowConfig = Cast<UCellFlowConfig>(PreviewActor->GetConfig())) {
			// Setup the grid flow graph asset to the currently edited asset
			CellFlowConfig->CellFlow = Cast<UCellFlowAsset>(AssetBeingEdited);
			CellFlowConfig->MaxBuildTimePerFrameMs = 30;
		}

		// Setup the default theme file
		static const TCHAR* DefaultThemeFileLocation = TEXT("/DungeonArchitect/Core/Runtime/Builders/GridFlowContent/Theme/T_DefaultGridFlow");
		UDungeonThemeAsset* Theme = Cast<UDungeonThemeAsset>(StaticLoadObject(UDungeonThemeAsset::StaticClass(), nullptr, DefaultThemeFileLocation));
		if (Theme) {
			PreviewActor->Themes.Reset();
			PreviewActor->Themes.Add(Theme);
		}
	}
	return PreviewActor;
}

FText FCellFlowEditor::GetEditorBrandingText() const {
    return LOCTEXT("EditorBranding", "CELL FLOW");
}

UFlowEditorSettings* FCellFlowEditor::CreateEditorSettingsObject() const {
	UCellFlowEditorSettings* NewSettings = NewObject<UCellFlowEditorSettings>();
	NewSettings->MaxBuildRetries = 100;
	return NewSettings;
}

FName FCellFlowEditor::GetToolkitFName() const {
	return FName("CellFlowEditor");
}

FText FCellFlowEditor::GetBaseToolkitName() const {
	return LOCTEXT("AppLabel", "Cell Flow Editor");
}

FString FCellFlowEditor::GetWorldCentricTabPrefix() const {
	return TEXT("CellFlowEditor");
}

FString FCellFlowEditor::GetReferencerName() const {
	static const FString NameString = TEXT("FCellFlowEditor");
	return NameString;
}

void FCellFlowEditor::ConfigureDomainObject(IFlowDomainPtr Domain) {
	if (!Domain.IsValid()) return;
	if (Domain->GetDomainID() == FCellFlowLayoutGraphDomain::DomainID) {
		// Configure the abstract 3d domain object from the editor settings
		if (const UCellFlowEditorSettings* CellFlowEditorSettings = Cast<UCellFlowEditorSettings>(EditorSettings)) {
			TSharedPtr<FCellFlowLayoutGraphDomain> CellFlowLayoutGraphDomain = StaticCastSharedPtr<FCellFlowLayoutGraphDomain>(Domain);

			// Configure the domain here, based on the editor config

		}
	}
}


#undef LOCTEXT_NAMESPACE

