//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FDungeonForgeEditorCommands : public TCommands<FDungeonForgeEditorCommands> {
public:
	FDungeonForgeEditorCommands();

	// ~Begin TCommands<> interface
	virtual void RegisterCommands() override;
	// ~End TCommands<> interface

	TSharedPtr<FUICommandInfo> ToggleEnabled;
	TSharedPtr<FUICommandInfo> ToggleDebug;
};

