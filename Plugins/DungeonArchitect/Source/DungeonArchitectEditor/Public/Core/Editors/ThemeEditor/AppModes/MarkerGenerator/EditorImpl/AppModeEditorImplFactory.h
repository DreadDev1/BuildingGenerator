//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

class UDungeonBuilder;
class IMGAppModeEditorInterface;

class FMGAppModeEditorImplFactory {
public:
	static TSharedPtr<IMGAppModeEditorInterface> Create(const TSubclassOf<UDungeonBuilder>& InBuilderClass);
};

