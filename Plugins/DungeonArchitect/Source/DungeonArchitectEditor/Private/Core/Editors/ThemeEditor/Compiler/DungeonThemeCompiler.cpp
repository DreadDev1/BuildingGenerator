//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/Compiler/DungeonThemeCompiler.h"

#include "Core/Editors/ThemeEditor/Compiler/Impl/DungeonThemeCompilerV2.h"

TSharedPtr<IDungeonThemeCompiler> IDungeonThemeCompiler::CreateLatestCompiler() {
	return MakeShared<FDungeonThemeCompilerV2>();
}
