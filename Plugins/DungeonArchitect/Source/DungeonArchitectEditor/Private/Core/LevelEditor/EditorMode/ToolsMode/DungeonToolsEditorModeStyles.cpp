//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ToolsMode/DungeonToolsEditorModeStyles.h"

#include "Core/LevelEditor/Customizations/DungeonArchitectStyle.h"

#include "Brushes/SlateImageBrush.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "SlateOptMacros.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/StyleColors.h"

#define PLUGIN_IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( FDungeonToolsEditorModeStyles::InContent( RelativePath, ".png" ), __VA_ARGS__ )
#define PLUGIN_IMAGE_BRUSH_SVG( RelativePath, ... ) FSlateVectorImageBrush( FDungeonToolsEditorModeStyles::InContent( RelativePath, ".svg" ), __VA_ARGS__ )
#define PLUGIN_BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( FDungeonToolsEditorModeStyles::InContent( RelativePath, ".png" ), __VA_ARGS__ )


FString FDungeonToolsEditorModeStyles::InContent(const FString& RelativePath, const ANSICHAR* Extension)
{
	static FString ResourcesDir = IPluginManager::Get().FindPlugin(TEXT("DungeonArchitect"))->GetBaseDir() / TEXT("Resources");
	return (ResourcesDir / RelativePath) + Extension;
}

TSharedPtr< FSlateStyleSet > FDungeonToolsEditorModeStyles::StyleSet = nullptr;
TSharedPtr< class ISlateStyle > FDungeonToolsEditorModeStyles::Get() { return StyleSet; }

FName FDungeonToolsEditorModeStyles::GetStyleSetName()
{
	static FName DungeonToolsStyleName(TEXT("DungeonToolsStyle"));
	return DungeonToolsStyleName;
}

const FSlateBrush* FDungeonToolsEditorModeStyles::GetBrush(FName PropertyName, const ANSICHAR* Specifier)
{
	return Get()->GetBrush(PropertyName, Specifier);
}


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FDungeonToolsEditorModeStyles::Initialize()
{
	// Const icon sizes
	const FVector2D Icon8x8(8.0f, 8.0f);
	const FVector2D Icon16x16(16.0f, 16.0f);
	const FVector2D Icon20x20(20.0f, 20.0f);
	const FVector2D Icon28x28(28.0f, 28.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);
	const FVector2D Icon120(120.0f, 120.0f);

	// Only register once
	if (StyleSet.IsValid())
	{
		return;
	}

	StyleSet = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

	// If we get asked for something that we don't set, we should default to editor style
	StyleSet->SetParentStyleName("EditorStyle");
	
	const FTextBlockStyle& NormalText = FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText");

	StyleSet->Set("DungeonToolsManagerCommands.LoadFavoritesTools", new PLUGIN_IMAGE_BRUSH_SVG("Icons/ToolsMode/LoadFavoritesTools", Icon20x20));
	StyleSet->Set("DungeonToolsManagerCommands.LoadGridTools", new PLUGIN_IMAGE_BRUSH_SVG("Icons/ToolsMode/LoadToolsDefault", Icon20x20));
	StyleSet->Set("DungeonToolsManagerCommands.LoadSnapTools", new PLUGIN_IMAGE_BRUSH_SVG("Icons/ToolsMode/LoadToolsDefault", Icon20x20));
	StyleSet->Set("DungeonToolsManagerCommands.LoadFlowTools", new PLUGIN_IMAGE_BRUSH_SVG("Icons/ToolsMode/LoadToolsDefault", Icon20x20));

	StyleSet->Set("DungeonToolsManagerCommands.LoadFlowTools", new PLUGIN_IMAGE_BRUSH_SVG("Icons/ToolsMode/LoadToolsDefault", Icon20x20));
	
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
};

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FDungeonToolsEditorModeStyles::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}


