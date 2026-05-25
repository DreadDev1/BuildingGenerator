//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FDungeonCanvasEditorCommands : public TCommands<FDungeonCanvasEditorCommands>
{
public:
	FDungeonCanvasEditorCommands();

	virtual void RegisterCommands() override;
public:
	TSharedPtr<FUICommandInfo> EditCanvasSettings;
	TSharedPtr<FUICommandInfo> EditDungeonSettings;
	TSharedPtr<FUICommandInfo> Compile;
	TSharedPtr<FUICommandInfo> RandomizeDungeon;
};

