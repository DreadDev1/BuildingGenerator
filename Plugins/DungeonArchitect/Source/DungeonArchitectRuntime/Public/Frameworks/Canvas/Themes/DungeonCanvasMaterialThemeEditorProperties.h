//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Utils/DungeonEditorViewportProperties.h"
#include "DungeonCanvasMaterialThemeEditorProperties.generated.h"

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonCanvasMaterialThemeEditorProperties : public UDungeonEditorViewportProperties {
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DungeonCanvas", meta = (UIMin = "0.0", UIMax = "100.0"))
	float LayoutDrawMarginPercent = 20;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	bool bRandomizeDungeonOnBuild = true;
};

