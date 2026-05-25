//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Styling/AppStyle.h"

class DUNGEONARCHITECTEDITOR_API FCellFlowDomainEdViewportCommands : public TCommands<FCellFlowDomainEdViewportCommands> {
public:

	FCellFlowDomainEdViewportCommands()
		: TCommands<FCellFlowDomainEdViewportCommands>
		(
			TEXT("CellFlowEditorContext"), // Context name for fast lookup
			NSLOCTEXT("Contexts", "CellFlowViewport", "Preview 3D"), // Localized context name for displaying
			NAME_None, //TEXT("EditorViewport"), // Parent context name.  
			FAppStyle::GetAppStyleSetName() // Icon Style Set
		) {
	}

	TSharedPtr<FUICommandInfo> ToggleDebugData;
	TSharedPtr<FUICommandInfo> DisplayDungeonProperties;

public:
	/** Registers our commands with the binding system */
	virtual void RegisterCommands() override;

};

