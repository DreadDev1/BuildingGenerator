//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Dungeon.h"
#include "ThemeEditorDungeonActor.generated.h"

UCLASS() //hidecategories=(Transform, "Dungeon Architect", "Build System"))
class AThemeEditorDungeonActor : public ADungeon {
	GENERATED_BODY()
public:
	void HandlePropertyChanged(const FName& InPropertyName);
	void InitFromPreviewViewportSettings(UDungeonEditorViewportProperties* InPreviewViewportProperties);
	
public:
	bool bPerformFullRebuild{};

private:
	TWeakObjectPtr<UDungeonEditorViewportProperties> PreviewViewportProperties;
};

