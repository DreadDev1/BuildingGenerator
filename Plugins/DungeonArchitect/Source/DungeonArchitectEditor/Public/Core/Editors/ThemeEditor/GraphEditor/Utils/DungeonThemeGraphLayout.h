//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

class SGraphEditor;
class UEdGraph;

class FDungeonThemeGraphLayout {
public:
	struct FSettings {
		FVector2D NodePadding = FVector2D(16, 24);
		float ClusterPadding = 60;
	};
	
	static const FSettings DefaultSettings;
	static void PerformLayout(UEdGraph* InThemeGraph, TSharedPtr<SGraphEditor> InGraphEditor, const FSettings& InSettings = DefaultSettings);
};

