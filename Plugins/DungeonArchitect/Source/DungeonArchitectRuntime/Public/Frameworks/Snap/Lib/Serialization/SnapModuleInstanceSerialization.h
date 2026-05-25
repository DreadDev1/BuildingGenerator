//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Layout/DungeonLayoutData.h"
#include "Core/Utils/DungeonShapes.h"
#include "Frameworks/Canvas/DungeonCanvasRoomShapeTexture.h"
#include "SnapModuleInstanceSerialization.generated.h"

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FSnapModuleInstanceSerializedData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FGuid ModuleInstanceId;

	UPROPERTY()
	FTransform WorldTransform;

	UPROPERTY()
	TSoftObjectPtr<UWorld> Level;

	UPROPERTY()
	TMap<FName, TSoftObjectPtr<UWorld>> ThemedLevels;
    
	UPROPERTY()
	FName Category;

	UPROPERTY()
	FBox ModuleBounds = FBox(ForceInitToZero);
    
	UPROPERTY()
	FDAShapeList ModuleBoundShapes;

	UPROPERTY()
	FDungeonCanvasRoomShapeTextureList CanvasRoomShapeTextures;

	UPROPERTY()
	TArray<FDungeonPointOfInterest> PointsOfInterest;
    
	TSoftObjectPtr<UWorld> GetThemedLevel(const FName& InThemeName) const;
};

