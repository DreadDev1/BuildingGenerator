//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ThemeMode/Common/DungeonThemeEdModeGizmoProxy.h"


///////////////////////////////// ADungeonThemeEdModeGizmoProxy /////////////////////////////////
ADungeonThemeEdModeGizmoProxy::ADungeonThemeEdModeGizmoProxy() {
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	
	// Set transient and editor-only flags
	bIsEditorOnlyActor = true;
}

void ADungeonThemeEdModeGizmoProxy::PostEditMove(bool bFinished) {
	Super::PostEditMove(bFinished);

	if (OnGizmoEditChanged.IsBound()) {
		OnGizmoEditChanged.Broadcast(bFinished);
	}
}

