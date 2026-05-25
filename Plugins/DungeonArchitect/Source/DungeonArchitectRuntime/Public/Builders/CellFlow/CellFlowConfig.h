//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/DungeonConfig.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowStructs.h"
#include "CellFlowConfig.generated.h"

class UCellFlowAsset;
class UMaterialInterface;

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FCellFlowMeshNoiseSettings {
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category="Noise")
	float NoiseScale = 400;
	
	UPROPERTY(EditAnywhere, Category="Noise")
	float NoiseAmplitudeMin = -10;
	
	UPROPERTY(EditAnywhere, Category="Noise")
	float NoiseAmplitudeMax = 40;
	
	UPROPERTY(EditAnywhere, Category="Noise")
	int32 NumOctaves = 15;
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FCellFlowMeshProfile {
	GENERATED_BODY()
	
	FCellFlowMeshProfile();

	UPROPERTY(EditAnywhere, Category="Mesh")
	TObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditAnywhere, Category="Mesh", meta=(UIMin=0, UIMax=8))
	int32 NumRenderSubDiv{0};

	UPROPERTY(EditAnywhere, Category="Mesh", meta=(UIMin=0, UIMax=8))
	int32 NumCollisionSubDiv{0};

	UPROPERTY(EditAnywhere, Category="Mesh")
	float LODFactorScale{1.0f};
	
	UPROPERTY(EditAnywhere, Category="Mesh")
	bool bApplyNoise{false};

	UPROPERTY(EditAnywhere, Category="Mesh", meta=(EditCondition="bApplyNoise"))
	FCellFlowMeshNoiseSettings NoiseSettings;
	
	UPROPERTY(EditAnywhere, Category="Mesh")
	bool bSmoothNormals{false};
}; 

UCLASS(BlueprintType)
class DUNGEONARCHITECTRUNTIME_API UCellFlowConfigMarkerSettings : public UDungeonConfigMarkerSettings {
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="Grid Cell Flow")
	FCellFlowGridMarkerSetupList DefaultGridPathMarkers;

	UPROPERTY(EditAnywhere, Category="Grid Cell Flow")
	TMap<FString, FCellFlowGridMarkerSetupList> GridPathMarkers;
	
	UPROPERTY(EditAnywhere, Category="Voronoi Cell Flow")
	FCellFlowVoronoiMarkerSetupList DefaultVoronoiPathMarkers;

	UPROPERTY(EditAnywhere, Category="Voronoi Cell Flow")
	TMap<FString, FCellFlowVoronoiMarkerSetupList> VoronoiPathMarkers;
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UCellFlowConfig : public UDungeonConfig {
	GENERATED_BODY()
	
public:	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
	TSoftObjectPtr<UCellFlowAsset> CellFlow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
	FVector GridSize = FVector(400, 400, 200);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
	int32 MaxRetries = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
	TMap<FString, FString> ParameterOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
	TSoftObjectPtr<UCellFlowConfigMarkerSettings> MarkerConfig;

	/** Should we generate the voronoi base mesh? */
	UPROPERTY(EditAnywhere, Category = "Voronoi Meshing", AdvancedDisplay, meta=(HideInDetailPanel))
	bool bGenerateVoronoiBaseMesh = false;

	/** The fallback mesh profile that would be used on chunks */
	UPROPERTY(EditAnywhere, Category = "Voronoi Meshing", AdvancedDisplay, meta=(HideInDetailPanel))
	FCellFlowMeshProfile DefaultVoronoiMeshProfile;

	/** Each path could have their own mesh profiles, so you can decorate them differently.  If a path's mesh profile is not provided here, the fallback mesh profile would be used */
	UPROPERTY(EditAnywhere, Category = "Voronoi Meshing", AdvancedDisplay, meta=(HideInDetailPanel))
	TMap<FString, FCellFlowMeshProfile> VoronoiPathMeshProfiles;

	virtual FDungeonConfigGenericSettings GatherGenericSettings() override;
};

