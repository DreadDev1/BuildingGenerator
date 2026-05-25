//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/LaunchPad/LaunchPad.h"

#include "Core/Editors/LaunchPad/Widgets/SLaunchPadWeb.h"
#include "Core/LevelEditor/Customizations/DungeonArchitectStyle.h"

#include "Framework/Docking/TabManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "LaunchPadSystem"

namespace {
    const FName DALaunchPadWindowID = FName(TEXT("DALaunchPadApp"));

    TSharedRef<class SDockTab> SpawnLaunchPadTab(const FSpawnTabArgs& Args) {
        return SNew(SDockTab)
            .TabRole(NomadTab)
            [
                SNew(SLaunchPadWeb)
            ];
    }
}

void FLaunchPadSystem::Register() {
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
                                DALaunchPadWindowID,
                                FOnSpawnTab::CreateStatic(&SpawnLaunchPadTab))
                            .SetDisplayName(LOCTEXT("TabTitle", "Launch Pad - Dungeon Architect"))
                            .SetTooltipText(LOCTEXT("TooltipText", "Browse Samples and Templates"))
                            .SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory())
                            .SetIcon(FSlateIcon(FDungeonArchitectStyle::GetStyleSetName(),
                                                "DungeonArchitect.ToolbarItem.IconLaunchPad"));
    
    float DPIScale = FPlatformApplicationMisc::GetDPIScaleFactorAtPoint(0, 0);
    FVector2D WindowSize = FVector2D(1920, 1080);
    if (DPIScale > 0.01f) {
        WindowSize /= DPIScale;
    }
    
    FGlobalTabmanager::Get()->RegisterDefaultTabWindowSize(DALaunchPadWindowID, WindowSize);
}

void FLaunchPadSystem::Unregister() {
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DALaunchPadWindowID);
}

void FLaunchPadSystem::Launch() {
    FGlobalTabmanager::Get()->TryInvokeTab(DALaunchPadWindowID);
}

#undef LOCTEXT_NAMESPACE

