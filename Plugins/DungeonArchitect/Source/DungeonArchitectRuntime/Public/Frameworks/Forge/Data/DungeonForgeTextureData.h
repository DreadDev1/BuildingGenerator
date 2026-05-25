//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/Common/DungeonForgePoint.h"
#include "Frameworks/Forge/Data/DungeonForgeData.h"
#include "DungeonForgeTextureData.generated.h"

class UTexture;
class UMaterialInterface;
class UDungeonSpatialSurfaceMapData;

UENUM(BlueprintType)
enum class EDungeonForgeTextureColorChannel : uint8
{
	Red,
	Green,
	Blue,
	Alpha
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeTextureData: public UDungeonForgeData {
	GENERATED_BODY()
public:
	UPROPERTY()
	TWeakObjectPtr<UTexture> DebugPreviewTexture;
	
	UPROPERTY()
	EDungeonForgeTextureColorChannel ColorChannel = EDungeonForgeTextureColorChannel::Red;
	
	UPROPERTY()
	float BaseScale = 1000;
	
	UPROPERTY()
	FTransform WorldTransform = FTransform::Identity;

	UPROPERTY()
	TSoftObjectPtr<UMaterialInterface> DebugMaterial;
	
	UPROPERTY()
	int32 Width = 0;
	
	UPROPERTY()
	int32 Height = 0;

	UPROPERTY()
	TArray<FDungeonForgePoint> Points;
	
public:
#if WITH_EDITOR
	virtual void WriteDebugData(UDungeonDebugVisualizer* InDebugVisualizer) const override;
#endif // WITH_EDITOR

	FTransform GetTextureWorldTransform() const;
};

