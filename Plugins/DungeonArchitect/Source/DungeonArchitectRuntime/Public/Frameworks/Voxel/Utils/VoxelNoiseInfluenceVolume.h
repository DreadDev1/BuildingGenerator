//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Voxel/Chunk/VoxelChunkDescriptor.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "VoxelNoiseInfluenceVolume.generated.h"

/**
 * Volume actor that designers can place in the world to define noise influence zones.
 * These zones locally override global noise settings with smooth transitions.
 */
UCLASS(Blueprintable, hidecategories = (Collision, Attachment, Actor, Input, Rendering))
class DUNGEONARCHITECTRUNTIME_API ADAVoxelNoiseInfluenceVolume : public AActor
{
	GENERATED_BODY()

public:
	ADAVoxelNoiseInfluenceVolume();

	// Influence zone data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Influence Zone", meta = (ShowOnlyInnerProperties))
	FDANoiseInfluenceZone InfluenceZoneData;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	// Visual components for editor visualization
	UPROPERTY()
	class UBoxComponent* InnerBoxComponent;

	UPROPERTY()
	class UBoxComponent* OuterBoxComponent;

	void UpdateVisualization();
};

