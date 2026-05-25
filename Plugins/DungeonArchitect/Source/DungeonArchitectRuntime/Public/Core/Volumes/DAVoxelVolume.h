//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Voxel/Chunk/VoxelChunkDescriptor.h"

#include "GameFramework/Actor.h"
#include "DAVoxelVolume.generated.h"

UCLASS(Category="Dungeon Architect", HideCategories=(Replication, HLODLayer, Actor, Input, Collision, Rendering, HLOD, WorldPartition, DataLayers, Cooking))
class DUNGEONARCHITECTRUNTIME_API ADAVoxelVolume : public AActor {
	GENERATED_BODY()
	
public:
	ADAVoxelVolume();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	
	UPROPERTY(EditAnywhere, Category="Voxel Volume")
	FVector BoxExtent = FVector(100.0f, 100.0f, 100.0f);
	
	UPROPERTY(EditAnywhere, Category="Voxel Volume", meta=(Tooltip="If true, carves out space (subtracts). If false, adds solid geometry."))
	bool bSubtract = true;

	UPROPERTY(EditAnywhere, Category="Voxel Volume")
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, Category="Voxel Volume", meta=(ClampMin="0", ClampMax="255", HideInDetailPanel))
	uint8 MaterialID = 0;
	
	UPROPERTY(EditAnywhere, Category="Voxel Volume")
	bool bOverrideNoiseSettings = false;
	
	UPROPERTY(EditAnywhere, Category="Voxel Volume", meta=(EditCondition="bOverrideNoiseSettings"))
	FDAVoxelNoiseSettings NoiseSettingsOverride;
	
	FDAVoxelShape CreateVoxelShape() const;
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
protected:
	virtual void BeginPlay() override;
	void InitializeVoxelVolume() const;
	
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<class UBoxComponent> VolumeComponent;
#endif
	
};

