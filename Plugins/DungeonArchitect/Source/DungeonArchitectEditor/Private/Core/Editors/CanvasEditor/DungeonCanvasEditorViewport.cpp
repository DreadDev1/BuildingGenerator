//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/CanvasEditor/DungeonCanvasEditorViewport.h"

#include "Core/Editors/CanvasEditor/DungeonCanvasMaterialThemeEditor.h"

#include "SEditorViewportToolBarButton.h"
#include "Widgets/SViewport.h"

#define LOCTEXT_NAMESPACE "SDungeonCanvasEditorViewport"

void SDungeonCanvasEditorViewport::Construct(const FArguments& InArgs, const TSharedRef<FDungeonCanvasMaterialThemeEditor>& InToolkit) {
	EditorPtr = InToolkit;

	SDungeonCanvasViewport::Construct(
		SDungeonCanvasViewport::FArguments()
		.DrawSettings(InArgs._DrawSettings)
	);
}

UDungeonCanvasComponent* SDungeonCanvasEditorViewport::GetDungeonCanvasInstance() const {
	TSharedPtr<FDungeonCanvasMaterialThemeEditor> DungeonCanvasEditor = EditorPtr.Pin();
	if (DungeonCanvasEditor.IsValid()) {
		return DungeonCanvasEditor->GetInstance();
	}
	return SDungeonCanvasViewport::GetDungeonCanvasInstance();
}

#undef LOCTEXT_NAMESPACE

