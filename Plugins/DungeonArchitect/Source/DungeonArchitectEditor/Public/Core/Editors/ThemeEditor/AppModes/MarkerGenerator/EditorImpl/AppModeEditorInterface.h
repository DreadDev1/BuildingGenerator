//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

class UMarkerGenLayer;
class IMGPatternEditor;

class IMGAppModeEditorInterface {
public:
	virtual ~IMGAppModeEditorInterface() {}
	virtual TSharedPtr<IMGPatternEditor> CreatePatternEditorImpl() = 0;
	virtual UMarkerGenLayer* CreateNewLayer(UObject* InOuter) = 0;
	virtual bool IsLayerCompatible(UMarkerGenLayer* InLayer) = 0;
};

