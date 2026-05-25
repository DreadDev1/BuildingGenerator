//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/FlowEditor/DomainEditors/Impl/CellFlow/Viewport3D/SCellFlowDomainEdViewport.h"

#include "Core/Editors/FlowEditor/DomainEditors/Impl/CellFlow/Viewport3D/CellFlowDomainEdViewportCommands.h"

#include "EditorViewportCommands.h"
#include "Framework/Application/SlateApplication.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"
#include "SEditorViewportViewMenu.h"

#define LOCTEXT_NAMESPACE "SCellFlowDomainEdViewport"

//////////////////////////////// SCellFlowDomainEdViewport ////////////////////////////////
void SCellFlowDomainEdViewport::Construct(const FArguments& InArgs) {
	SFlowDomainEdViewport3D::Construct(SFlowDomainEdViewport3D::FArguments());
	
}

TSharedPtr<SWidget> SCellFlowDomainEdViewport::BuildViewportToolbar() {
	// Build our toolbar level toolbar
	const TSharedRef<SCellFlowDomainEdViewportToolbar> ToolBar = SNew(SCellFlowDomainEdViewportToolbar)
		.Viewport(SharedThis(this))
		.Visibility(this, &SCellFlowDomainEdViewport::GetToolbarVisibility)
		.IsEnabled(FSlateApplication::Get().GetNormalExecutionAttribute());

	return
		SNew(SVerticalBox)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+SVerticalBox::Slot()
		  .AutoHeight()
		  .VAlign(VAlign_Top)
		[
			ToolBar
		];
}

EVisibility SCellFlowDomainEdViewport::GetToolbarVisibility() const {
	return EVisibility::Visible;
}


//////////////////////////////// SCellFlowDomainEdViewportToolbar ////////////////////////////////
void SCellFlowDomainEdViewportToolbar::Construct(const FArguments& InArgs) {
    Viewport = InArgs._Viewport;

    TSharedRef<SCellFlowDomainEdViewport> ViewportRef = Viewport.Pin().ToSharedRef();
    const FMargin ToolbarSlotPadding(2.0f, 2.0f);

    static const FName DefaultForegroundName("DefaultForeground");

    ChildSlot
    [
        SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("NoBorder"))
		// Color and opacity is changed based on whether or not the mouse cursor is hovering over the toolbar area
		//.ColorAndOpacity(this, &SViewportToolBar::OnGetColorAndOpacity)
		.ForegroundColor(FAppStyle::GetSlateColor(DefaultForegroundName))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                  .AutoWidth()
                  .Padding(ToolbarSlotPadding)
                [
                    SNew(SEditorViewportToolbarMenu)
					.Label(LOCTEXT("ShowMenuTitle", "Properties"))
					.Cursor(EMouseCursor::Default)
					.ParentToolBar(SharedThis(this))
					.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("EditorViewportToolBar.ShowMenu")))
					.OnGetMenuContent(this, &SCellFlowDomainEdViewportToolbar::GeneratePropertiesMenu)
                ]
                + SHorizontalBox::Slot()
                  .AutoWidth()
                  .Padding(ToolbarSlotPadding)
                [
                    SNew(SEditorViewportToolbarMenu)
					.Label(LOCTEXT("ShowCamMenuTitle", "Camera"))
					.Cursor(EMouseCursor::Default)
					.ParentToolBar(SharedThis(this))
					.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("EditorViewportToolBar.CameraMenu")))
					.OnGetMenuContent(this, &SCellFlowDomainEdViewportToolbar::GenerateCameraMenu)
                ]
                + SHorizontalBox::Slot()
                  .AutoWidth()
                  .Padding(ToolbarSlotPadding)
                [
                    SNew(SEditorViewportViewMenu, ViewportRef, SharedThis(this))
					.Cursor(EMouseCursor::Default)
					.MenuExtenders(GetViewMenuExtender())
					.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ViewMenuButton")))
                ]
            ]
        ]
    ];

    SViewportToolBar::Construct(SViewportToolBar::FArguments());
}


TSharedRef<SWidget> SCellFlowDomainEdViewportToolbar::GeneratePropertiesMenu() const {
    const bool bInShouldCloseWindowAfterMenuSelection = true;
    FMenuBuilder PropertiesMenuBuilder(bInShouldCloseWindowAfterMenuSelection, Viewport.Pin()->GetCommandList());

    const FCellFlowDomainEdViewportCommands& PreviewViewportActions = FCellFlowDomainEdViewportCommands::Get();
    PropertiesMenuBuilder.BeginSection("CellFlowOptions");
    PropertiesMenuBuilder.AddMenuEntry(PreviewViewportActions.ToggleDebugData);
    PropertiesMenuBuilder.AddMenuEntry(PreviewViewportActions.DisplayDungeonProperties);
    PropertiesMenuBuilder.EndSection();

    return PropertiesMenuBuilder.MakeWidget();
}


TSharedRef<SWidget> SCellFlowDomainEdViewportToolbar::GenerateCameraMenu() const {
    const bool bInShouldCloseWindowAfterMenuSelection = true;
    FMenuBuilder CameraMenuBuilder(bInShouldCloseWindowAfterMenuSelection, Viewport.Pin()->GetCommandList());

    // Camera types
    CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Perspective);

    CameraMenuBuilder.BeginSection("LevelViewportCameraType_Ortho", LOCTEXT("CameraTypeHeader_Ortho", "Orthographic"));
    CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Top);
    CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Bottom);
    CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Left);
    CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Right);
    CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Front);
    CameraMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Back);
    CameraMenuBuilder.EndSection();

    return CameraMenuBuilder.MakeWidget();
}

TSharedPtr<FExtender> SCellFlowDomainEdViewportToolbar::GetViewMenuExtender() {
    FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
    TSharedPtr<FExtender> LevelEditorExtenders = LevelEditorModule.GetMenuExtensibilityManager()->GetAllExtenders();
    return LevelEditorExtenders;
}

#undef LOCTEXT_NAMESPACE

