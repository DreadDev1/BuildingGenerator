//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/ThemeEditor/Compiler/DungeonThemeCompiler.h"

class UEdGraph_DungeonTheme;

class FDungeonThemeCompilerV1 : public IDungeonThemeCompiler {
public:
	virtual bool Compile(UDungeonThemeAsset* InThemeAsset, TArray<FDungeonThemeGraphBuildError>& OutErrors) override;

private:
	static void RebuildGraph(UDungeonThemeAsset* InThemeAsset, TArray<FDungeonThemeVisualNodeData>& OutProps, TArray<FDungeonThemeGraphBuildError>& OutErrors);
};

