//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FDungeonForgeEditorModeCommands : public TCommands<FDungeonForgeEditorModeCommands>
{
public:
	FDungeonForgeEditorModeCommands();

	virtual void RegisterCommands() override;
	static TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> GetCommands();
	static TMap<FName, TSharedPtr<FUICommandInfo>> GetCategories();
	
	TSharedPtr<FUICommandInfo> CommonToolsCategory;
	TSharedPtr<FUICommandInfo> GridToolsCategory;
	TSharedPtr<FUICommandInfo> TextureToolsCategory;

	TSharedPtr<FUICommandInfo> TransformTool;
	TSharedPtr<FUICommandInfo> GridPaintTool;
	TSharedPtr<FUICommandInfo> TexturePaintTool;

	TSharedPtr<FUICommandInfo> CreateLayerGrid2D;
	TSharedPtr<FUICommandInfo> CreateLayerGrid3D;
	TSharedPtr<FUICommandInfo> CreateLayerTexture;
	TSharedPtr<FUICommandInfo> CreateLayerFolder;
	
protected:
	TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> Commands;
	TMap<FName, TSharedPtr<FUICommandInfo>> Categories;
};

