//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FDungeonThemeEdModeCommands : public TCommands<FDungeonThemeEdModeCommands>
{
public:
	FDungeonThemeEdModeCommands();

	virtual void RegisterCommands() override;
	static TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> GetCommands();

	TSharedPtr<FUICommandInfo> SelectTool;
	TSharedPtr<FUICommandInfo> VisualNodeTool;
	TSharedPtr<FUICommandInfo> MarkerNodeTool;

protected:
	TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> Commands;
};
