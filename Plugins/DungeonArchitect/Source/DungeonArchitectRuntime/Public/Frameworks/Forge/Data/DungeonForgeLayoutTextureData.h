//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Canvas/DungeonCanvasRendering.h"
#include "Frameworks/Forge/Data/DungeonForgeData.h"
#include "DungeonForgeLayoutTextureData.generated.h"

class UMaterialInterface;

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeLayoutTextureData : public UDungeonForgeData {
	GENERATED_BODY()
public:
	UPROPERTY()
	FDungeonCanvasLayoutFloorTextures FloorTextures;
	
	UPROPERTY()
	FBox LocalBounds;
	
	UPROPERTY()
	FTransform WorldTransform;

	UPROPERTY()
	TSoftObjectPtr<UMaterialInterface> DebugMaterial;
	
#if WITH_EDITOR
	virtual void WriteDebugData(UDungeonDebugVisualizer* InDebugVisualizer) const override;
#endif // WITH_EDITOR

public:
	FTransform GetTextureWorldTransform() const;
};

