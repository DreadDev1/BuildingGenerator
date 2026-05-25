//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/DungeonCanvasMaterialLayer.h"

#include "Frameworks/Canvas/Themes/DungeonCanvasMaterialTheme.h"

int32 UDungeonCanvasMaterialLayer::GetLayerIndex() {
	int32 LayerIndex = INDEX_NONE;
	if (UDungeonCanvasMaterialTheme* Owner = Cast<UDungeonCanvasMaterialTheme>(GetOuter())) {
		for (int i = Owner->MaterialLayers.Num() - 1; i >= 0; i--) {
			LayerIndex++;
			TObjectPtr<UDungeonCanvasMaterialLayer> Layer = Owner->MaterialLayers[i];
			if (Layer == this) {
				return LayerIndex;
			}
		}
	}

	return INDEX_NONE;
}

