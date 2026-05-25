//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class DUNGEONARCHITECTEDITOR_API FDungeonArchitectCommands : public TCommands<FDungeonArchitectCommands> {
public:
    FDungeonArchitectCommands();

    virtual void RegisterCommands() override;

public:
    TSharedPtr<FUICommandInfo> OpenSupportForums;
    TSharedPtr<FUICommandInfo> OpenSupportDiscord;
    TSharedPtr<FUICommandInfo> OpenDocumentationWebsite;

    /** Commands for the dungeon editor mode toolbar. */
    TSharedPtr<FUICommandInfo> ModePaint;
    TSharedPtr<FUICommandInfo> ModeRectangle;
    TSharedPtr<FUICommandInfo> ModeBorder;
    TSharedPtr<FUICommandInfo> ModeSelect;

    TSharedPtr<FUICommandInfo> UpgradeThemeFile;
};

