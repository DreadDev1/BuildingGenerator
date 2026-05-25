//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/Legacy/DungeonLegacyEdModeToolkit.h"

#include "Core/LevelEditor/EditorMode/Legacy/DungeonLegacyEdMode.h"

#include "Editor.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "DungeonEditMode"

namespace DAEdModeImpl
{
    static const FName DAPaletteName(TEXT("Dungeon Architect")); 
    const TArray<FName> DAPaletteNames = { DAPaletteName };
}


void FDungeonLegacyEdModeToolkit::Init(const TSharedPtr<class IToolkitHost>& InitToolkitHost) {
    FModeToolkit::Init(InitToolkitHost);
}

FName FDungeonLegacyEdModeToolkit::GetToolkitFName() const {
    return FName("DungeonEditMode");
}

FText FDungeonLegacyEdModeToolkit::GetBaseToolkitName() const {
    return LOCTEXT("ToolkitName", "Dungeon Edit Mode");
}

class FEdMode* FDungeonLegacyEdModeToolkit::GetEditorMode() const {
    return GLevelEditorModeTools().GetActiveMode(FEdModeDungeonLegacy::EM_Dungeon);
}

TSharedPtr<SWidget> FDungeonLegacyEdModeToolkit::GetInlineContent() const {
    return DungeonEdWidget;
}

void FDungeonLegacyEdModeToolkit::GetToolPaletteNames(TArray<FName>& InPaletteName) const {
    InPaletteName = DAEdModeImpl::DAPaletteNames;
}

FText FDungeonLegacyEdModeToolkit::GetToolPaletteDisplayName(FName PaletteName) const {
    if (PaletteName == DAEdModeImpl::DAPaletteName)
    {
        return LOCTEXT("DAPaletteTitle", "Dungeon Architect");
    }
    return FText();
}

void FDungeonLegacyEdModeToolkit::BuildToolPalette(FName PaletteName, FToolBarBuilder& ToolbarBuilder) {
    if (PaletteName == DAEdModeImpl::DAPaletteName && DungeonEdWidget.IsValid())
    {
        //DungeonEdWidget->CustomizeToolBarPalette(ToolbarBuilder);
    }
}

FText FDungeonLegacyEdModeToolkit::GetActiveToolDisplayName() const {
    return LOCTEXT("ActiveToolDebugTitle", "Tool Name");
    //return DungeonEdWidget->GetActiveToolName();
}

FText FDungeonLegacyEdModeToolkit::GetActiveToolMessage() const {
    //return DungeonEdWidget->GetActiveToolMessage();
    return LOCTEXT("ActiveToolDebugTitle", "Tool Message");
}


void FDungeonLegacyEdModeToolkit::SetInlineContent(TSharedPtr<SWidget> Widget) {
    DungeonEdWidget = Widget;
}

#undef LOCTEXT_NAMESPACE

