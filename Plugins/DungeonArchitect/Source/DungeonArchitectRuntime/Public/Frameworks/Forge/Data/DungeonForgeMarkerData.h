//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/DungeonBuilder.h"
#include "Core/Markers/DungeonMarker.h"
#include "Frameworks/Forge/Data/DungeonForgeData.h"
#include "DungeonForgeMarkerData.generated.h"

UCLASS()
class UDungeonForgeMarkerData : public UDungeonForgeData {
	GENERATED_BODY()
public:
	UPROPERTY()
	int32 Seed = 0;
	
	UPROPERTY()
    TArray<FDungeonMarkerInstance> Markers;
	
	UPROPERTY()
	FTransform BaseTransform = FTransform::Identity;
	
	UPROPERTY()
	FGuid ChunkId;

	UPROPERTY(Transient)
	TObjectPtr<UDungeonBuilder> DungeonBuilder;
	
	UPROPERTY(Transient)
	TObjectPtr<UDungeonModel> DungeonModel;
	
	UPROPERTY(Transient)
	TObjectPtr<UDungeonConfig> DungeonConfig;
};

