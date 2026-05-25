//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Canvas/UI/Viewport/SDungeonCanvasViewport.h"

class FDungeonCanvasMaterialThemeEditor;
class UDungeonCanvasComponent;

class SDungeonCanvasEditorViewport : public SDungeonCanvasViewport {
public:
	SLATE_BEGIN_ARGS(SDungeonCanvasEditorViewport) {}
		SLATE_ATTRIBUTE(FDungeonCanvasDrawSettings, DrawSettings)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<FDungeonCanvasMaterialThemeEditor>& InToolkit);
	virtual UDungeonCanvasComponent* GetDungeonCanvasInstance() const override;

private:
	TWeakPtr<FDungeonCanvasMaterialThemeEditor> EditorPtr;
};

