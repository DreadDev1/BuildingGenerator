//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/Customizations/DungeonForgeEditorCustomization.h"

#include "Core/DungeonConfig.h"
#include "Core/LevelEditor/Customizations/DetailsCustomizationHelpers.h"
#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorMode.h"
#include "Frameworks/Forge/Elements/DungeonForgeCreateDungeonChunk.h"
#include "Frameworks/Forge/Elements/DungeonForgeStaticTextureElement.h"
#include "Frameworks/Forge/Elements/Interfaces/DungeonForgePaintBrushToolHandler.h"

#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "EditorModeManager.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "DungeonForgeEditorNodeCustomization"

void FDungeonForgeEditorNodeCustomizationBase::RegisterToolButton(IDetailLayoutBuilder& InDetailBuilder, const FText& InFilterText, const FText& InText, const TFunction<FReply()>& InCallback) {
	IDetailCategoryBuilder& ToolsCategory = GetToolsCategory(InDetailBuilder);
	ToolsCategory.AddCustomRow(InFilterText)
	.WholeRowContent()
	[
		SNew(SButton)
			.Text(InText)
			.VAlign(VAlign_Center)
			.OnClicked(FOnClicked::CreateLambda([=]() { return InCallback(); }))
	];
}

IDetailCategoryBuilder& FDungeonForgeEditorNodeCustomizationBase::GetToolsCategory(IDetailLayoutBuilder& DetailBuilder) {
	return DetailBuilder.EditCategory(TEXT("Tools"), FText::GetEmpty(), ECategoryPriority::Important);
}

UDungeonForgeEditorMode* FDungeonForgeEditorNodeCustomizationBase::GetForgeEditorMode() {
	return Cast<UDungeonForgeEditorMode>(GLevelEditorModeTools().GetActiveScriptableMode(UDungeonForgeEditorMode::EM_DungeonForgeEditorModeId));
}

///////////////////////// FDungeonForgeEditorNodeCustomization_CreateGridChunk /////////////////////////
void FDungeonForgeEditorNodeCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) {
	UDungeonForgeNodeSettings* NodeSettings= DADetailsCustomizationHelpers::GetEditedObject<UDungeonForgeNodeSettings>(DetailBuilder);

	// Show builder config for chunk nodes
	if (UDungeonForgeCreateDungeonChunkBaseSettings* ChunkNodeSettings = Cast<UDungeonForgeCreateDungeonChunkBaseSettings>(NodeSettings)) {
		if (ChunkNodeSettings->DungeonConfig) {
			DADetailsCustomizationHelpers::ShowDungeonConfigProperties(DetailBuilder, ChunkNodeSettings->DungeonConfig);
		}
	}
	
	TWeakObjectPtr<UDungeonForgeNodeSettings> WeakNodeSettings = NodeSettings;
	if (Cast<IDungeonForgeTransformToolHandler>(NodeSettings)) {
		RegisterToolButton(DetailBuilder,
			LOCTEXT("NodeTools_Filter_Location", "tool transform location"),
			LOCTEXT("NodeTools_Location", "Transform Tool"),
			[this, WeakNodeSettings]() {
				if (WeakNodeSettings.IsValid()) {
					if (UDungeonForgeEditorMode* ForgeMode = GetForgeEditorMode()) {
						ForgeMode->ActiveTransformTool(WeakNodeSettings.Get());
					}
				}
				return FReply::Handled();
			});
	}

	if (Cast<IDungeonForgePaintBrushToolHandler>(NodeSettings)) {
		RegisterToolButton(DetailBuilder,
			LOCTEXT("NodeTools_Filter_PaintBrush", "tool paint brush texture"),
			LOCTEXT("NodeTools_PaintBrush", "Texture Paint Tool"),
			[this, WeakNodeSettings]() {
				if (WeakNodeSettings.IsValid()) {
					if (UDungeonForgeEditorMode* ForgeMode = GetForgeEditorMode()) {
						ForgeMode->ActivePaintBrushTool(WeakNodeSettings.Get());
					}
				}
				return FReply::Handled();
			});
	}

	
	if (Cast<IDungeonForgeRandomizeToolHandler>(NodeSettings)) {
		RegisterToolButton(DetailBuilder,
			LOCTEXT("NodeTools_Filter_Location", "tool randomize seed"),
			LOCTEXT("NodeTools_Location", "Randomize Seed"),
			[WeakNodeSettings]() {
				if (WeakNodeSettings.IsValid()) {
					IDungeonForgeRandomizeToolHandler::Execute_RandomizeSeed(WeakNodeSettings.Get());
					
					// Request a rebuild
					if (UDungeonForgeEditorMode* ForgeMode = GetForgeEditorMode()) {
						ForgeMode->BuildDungeon();
					}
				}
				return FReply::Handled();
			});
	}
}

TSharedRef<IDetailCustomization> FDungeonForgeEditorNodeCustomization::MakeInstance() {
	return MakeShared<FDungeonForgeEditorNodeCustomization>();
}

#undef LOCTEXT_NAMESPACE

