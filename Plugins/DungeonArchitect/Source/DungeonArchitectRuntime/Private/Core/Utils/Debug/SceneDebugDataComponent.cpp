//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Utils/Debug/SceneDebugDataComponent.h"


void UDASceneDebugDataComponent::ClearDebugData() {
	Data = {};
	Modify();
}

