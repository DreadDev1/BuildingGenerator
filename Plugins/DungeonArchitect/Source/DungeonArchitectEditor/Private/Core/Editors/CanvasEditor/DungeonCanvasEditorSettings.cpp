//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/CanvasEditor/DungeonCanvasEditorSettings.h"

#include "Frameworks/Canvas/UI/Viewport/SDungeonCanvasViewport.h"

UDungeonCanvasEditorSettings::UDungeonCanvasEditorSettings( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
	, Background(EDungeonCanvasEditorBackgrounds::Checkered)
	, Sampling(EDungeonCanvasEditorSampling::Default)
	, BackgroundColor(FColor::FromHex("242424FF"))
	, CheckerColorOne(FColor(128, 128, 128))
	, CheckerColorTwo(FColor(64, 64, 64))
	, CheckerSize(16)
	, ZoomMode(EDungeonCanvasViewportZoomMode::Fit)
	, TextureBorderColor(FColor::White)
	, TextureBorderEnabled(true)
{ }

