//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

/**
 * TInteractiveToolCommands implementation for Modeling Mode Tools
 */
class DUNGEONARCHITECTEDITOR_API FDungeonToolsEditorCommands : public TCommands<FDungeonToolsEditorCommands>
{
public:
	FDungeonToolsEditorCommands();

protected:
	struct FStartToolCommand
	{
		FString ToolUIName;
		TSharedPtr<FUICommandInfo> ToolCommand;
	};
	TArray<FStartToolCommand> RegisteredTools;		// Tool start-commands listed below are stored in this list

	struct FDynamicExtensionCommand
	{
		FName RegistrationName;
		TSharedPtr<FUICommandInfo> Command;
	};
	TArray<FDynamicExtensionCommand> ExtensionPaletteCommands;

public:
	/**
	 * Find Tool start-command below by registered name (tool icon name in Mode palette)
	 */
	TSharedPtr<FUICommandInfo> FindToolByName(FString Name, bool& bFound) const;

	// Layout Segment tools
	TSharedPtr<FUICommandInfo> BeginLayoutSegmentManageTool;

	// Grid builder tools
	TSharedPtr<FUICommandInfo> BeginGridPaintTool;

	// Snap tools
	TSharedPtr<FUICommandInfo> BeginSnapStitchModuleTool;
	TSharedPtr<FUICommandInfo> BeginSnapConnectionTool;

	// Flow tools
	TSharedPtr<FUICommandInfo> BeginFlowNodeTool;
	
	TSharedPtr<FUICommandInfo> AddToFavorites;
	TSharedPtr<FUICommandInfo> RemoveFromFavorites;
	TSharedPtr<FUICommandInfo> LoadFavoritesTools;
	TSharedPtr<FUICommandInfo> LoadLayoutTools;

    TSharedPtr<FUICommandInfo> LoadGridTools;
    TSharedPtr<FUICommandInfo> LoadSnapTools;
    TSharedPtr<FUICommandInfo> LoadFlowTools;
    
    //
    // Accept/Cancel/Complete commands are used to end the active Tool via ToolManager
    //

    TSharedPtr<FUICommandInfo> AcceptActiveTool;
    TSharedPtr<FUICommandInfo> CancelActiveTool;
    TSharedPtr<FUICommandInfo> CompleteActiveTool;

    TSharedPtr<FUICommandInfo> AcceptOrCompleteActiveTool;
	TSharedPtr<FUICommandInfo> CancelOrCompleteActiveTool;

	/**
	 * Initialize above commands
	 */
	virtual void RegisterCommands() override;


	/**
	 * Dynamically register a new UI Command based on the given Name/Label/Tooltip/Icon.
	 * This is intended to be used outside of RegisterCommands(), eg by Modeling Mode extensions
	 * loaded when the Toolkit is created.
	 */
	static TSharedPtr<FUICommandInfo> RegisterExtensionPaletteCommand(
		FName Name,
		const FText& Label,
		const FText& Tooltip,
		const FSlateIcon& Icon);

};

