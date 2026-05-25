//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/CanvasEditor/DungeonCanvasEditorToolbar.h"

#include "Core/Editors/CanvasEditor/DungeonCanvasEditorCommands.h"

#include "ToolMenu.h"
#include "ToolMenuSection.h"

#define LOCTEXT_NAMESPACE "DungeonCanvasBlueprintEditorToolbar"

FDungeonCanvasBlueprintEditorToolbar::FDungeonCanvasBlueprintEditorToolbar()
{
	
}

void FDungeonCanvasBlueprintEditorToolbar::AddDungeonCanvasBlueprintEditorModesToolbar(TSharedPtr<FExtender> Extender)
{
	{
		/*
		Extender->AddToolBarExtension(
			"Asset",
			EExtensionHook::After,
			BlueprintEditor->GetToolkitCommands(),
			FToolBarExtensionDelegate::CreateSP(this, &FDungeonCanvasBlueprintEditorToolbar::FillDungeonCanvasBlueprintEditorModesToolbar));
		*/
	}
}

void FDungeonCanvasBlueprintEditorToolbar::AddCanvasGraphModeToolbar(UToolMenu* InMenu)
{
	/*
	{
		FToolMenuSection& BuilderSection = InMenu->FindOrAddSection("Builder");
		BuilderSection.InsertPosition = FToolMenuInsert("Asset", EToolMenuInsertType::After);

		BuilderSection.AddDynamicEntry("DungeonCanvasBuilderCombo",
			FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection) {
				const UBlueprintEditorToolMenuContext* Context = InSection.FindContext<UBlueprintEditorToolMenuContext>();
				TSharedPtr<FDungeonCanvasEditor> Editor = StaticCastSharedPtr<FDungeonCanvasEditor>(Context->BlueprintEditor.Pin());
				auto FnGetBuilder = [Editor]() -> TSubclassOf<UDungeonBuilder> {
					if (Editor.IsValid()) {
						if (const UDungeonCanvasBlueprint* CanvasBlueprint = Editor->GetDungeonCanvasBlueprint()) {
							if (CanvasBlueprint->PreviewDungeonProperties) {
								return CanvasBlueprint->PreviewDungeonProperties->BuilderClass;
							}
						}
					}

					// fallback to the grid builder
					return UGridDungeonBuilder::StaticClass();
				};
				
				TSharedPtr<SDungeonBuilderClassCombo> BuilderClassCombo = SNew(SDungeonBuilderClassCombo)
					.SelectedBuilderItem(FnGetBuilder())
					.OnSetDungeonBuilder_Lambda([Editor](TSubclassOf<UDungeonBuilder> InBuilder) {
						if (Editor.IsValid()) {
							if (const UDungeonCanvasBlueprint* CanvasBlueprint = Editor->GetDungeonCanvasBlueprint()) {
								if (CanvasBlueprint->PreviewDungeonProperties) {
									CanvasBlueprint->PreviewDungeonProperties->BuilderClass = InBuilder;
								
									FName PropertyName = GET_MEMBER_NAME_CHECKED(UDungeonCanvasEditorViewportProperties, BuilderClass);
									Editor->OnPropertyChanged(PropertyName.ToString(), CanvasBlueprint->PreviewDungeonProperties);
								}
							}
						}
					});
						
				InSection.AddEntry(FToolMenuEntry::InitWidget(
					"DungeonBuilder",
					BuilderClassCombo.ToSharedRef(),
					LOCTEXT("DungeonBuilder", "Dungeon Builder")));
				
			}));
	}
	*/

	{
		FToolMenuSection& ToolsSection = InMenu->AddSection("Tools");
		ToolsSection.InsertPosition = FToolMenuInsert("Asset", EToolMenuInsertType::After);
	
		ToolsSection.AddEntry(FToolMenuEntry::InitToolBarButton(
			FDungeonCanvasEditorCommands::Get().EditCanvasSettings,
			TAttribute<FText>(),
			TAttribute<FText>(),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Settings")
		));

	
		ToolsSection.AddEntry(FToolMenuEntry::InitToolBarButton(
			FDungeonCanvasEditorCommands::Get().EditDungeonSettings,
			TAttribute<FText>(),
			TAttribute<FText>(),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Settings")
		));
	}
}

void FDungeonCanvasBlueprintEditorToolbar::OnBuilderChanged() const {
	//if (BuilderClassCombo.IsValid()) {
	//	BuilderClassCombo->SetSelectedItem(FnGetBuilder());
	//}
}

/*
void FDungeonCanvasBlueprintEditorToolbar::AddListingModeToolbar(UToolMenu* InMenu)
{
	FToolMenuSection& Section = InMenu->AddSection("Tools");
	Section.InsertPosition = FToolMenuInsert("Asset", EToolMenuInsertType::After);

}


void FDungeonCanvasBlueprintEditorToolbar::FillDungeonCanvasBlueprintEditorModesToolbar(FToolBarBuilder& ToolbarBuilder)
{
	if (const TSharedPtr<FDungeonCanvasEditor> BlueprintEditor = BlueprintEditorWeakPtr.Pin())
	{
		UBlueprint* BlueprintObj = BlueprintEditor->GetBlueprintObj();
		if (!BlueprintObj || (!FBlueprintEditorUtils::IsLevelScriptBlueprint(BlueprintObj) && !FBlueprintEditorUtils::IsInterfaceBlueprint(BlueprintObj) && !BlueprintObj->bIsNewlyCreated))
		{
			TAttribute<FName> GetActiveMode(BlueprintEditor.ToSharedRef(), &FBlueprintEditor::GetCurrentMode);
			FOnModeChangeRequested SetActiveMode = FOnModeChangeRequested::CreateSP(BlueprintEditor.ToSharedRef(), &FBlueprintEditor::SetCurrentMode);

			// Left side padding
			BlueprintEditor->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(4.0f, 1.0f)));

			BlueprintEditor->AddToolbarWidget(
				SNew(SModeWidget, FDungeonCanvasApplicationModes::GetLocalizedMode(FDungeonCanvasApplicationModes::ListingMode), FDungeonCanvasApplicationModes::ListingMode)
				.OnGetActiveMode(GetActiveMode)
				.OnSetActiveMode(SetActiveMode)
				.ToolTip(IDocumentation::Get()->CreateToolTip(
					LOCTEXT("ListingModeButtonTooltip", "Switch to Job Listing Mode"),
					nullptr,
					TEXT("Shared/Editors/BlueprintEditor"),
					TEXT("ListingMode")))
				.IconImage(FAppStyle::GetBrush("BTEditor.Graph.NewTask"))
				.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ListingMode")))
			);

			BlueprintEditor->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(10.0f, 1.0f)));

			BlueprintEditor->AddToolbarWidget(
				SNew(SModeWidget, FDungeonCanvasApplicationModes::GetLocalizedMode(FDungeonCanvasApplicationModes::LogicMode), FDungeonCanvasApplicationModes::LogicMode)
				.OnGetActiveMode(GetActiveMode)
				.OnSetActiveMode(SetActiveMode)
				.CanBeSelected(BlueprintEditor.Get(), &FBlueprintEditor::IsEditingSingleBlueprint)
				.ToolTip(IDocumentation::Get()->CreateToolTip(
					LOCTEXT("LogicModeButtonTooltip", "Switch to Logic Editing Mode"),
					nullptr,
					TEXT("Shared/Editors/BlueprintEditor"),
					TEXT("GraphMode")))
				.IconImage(FAppStyle::GetBrush("Icons.Blueprint"))
				.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("LogicMode")))
			);

			// Right side padding
			BlueprintEditor->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(10.0f, 1.0f)));
		}
	}
}
*/


#undef LOCTEXT_NAMESPACE

