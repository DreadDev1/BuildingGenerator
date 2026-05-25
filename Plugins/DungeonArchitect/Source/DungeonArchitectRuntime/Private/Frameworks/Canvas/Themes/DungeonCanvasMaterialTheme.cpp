//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/Themes/DungeonCanvasMaterialTheme.h"

#include "Frameworks/Canvas/Themes/DungeonCanvasMaterialThemeEditorProperties.h"

UDungeonCanvasMaterialTheme::UDungeonCanvasMaterialTheme() {
	PreviewDungeonProperties = CreateDefaultSubobject<UDungeonCanvasMaterialThemeEditorProperties>("PreviewDungeonProperties");
}

