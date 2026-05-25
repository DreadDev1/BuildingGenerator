//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/ThemeEditor/Compiler/DungeonThemeCompiler.h"

struct FDungeonThemeCompiledGraph;

class FDungeonThemeCompilerV2 : public IDungeonThemeCompiler {
public:
	virtual bool Compile(UDungeonThemeAsset* InThemeAsset, TArray<FDungeonThemeGraphBuildError>& OutErrors) override;

private:
	static void CompiledGraph(UDungeonThemeAsset* InThemeAsset, TArray<FDungeonThemeGraphBuildError>& OutErrors);
};

