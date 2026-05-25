//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Voxel/Shaders/VoxelDensityComputeShader.h"

#include "Engine/Texture2D.h"
#include "UObject/NoExportTypes.h"
#include "VoxelSDFModel.generated.h"

struct FDAVoxelChunkDescriptor;
struct FDAVoxelShape;
struct FDungeonLayoutData;
struct FDungeonLayoutDataChunkInfo;
struct FRandomStream;
class FDungeonSceneProviderCommandList;
class AActor;
class ADungeon;

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonVoxelVerticalOffsets
{
	GENERATED_BODY()

	/** Additional space needed above the base geometry (e.g., cave ceiling) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Voxel)
	float TopOffset = 0.0f;

	/** Additional space needed below the base geometry (e.g., island platform depth) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Voxel)
	float BottomOffset = 0.0f;
};

UENUM(BlueprintType)
enum class EDungeonVoxelWorldType : uint8
{
	Cave UMETA(DisplayName = "Cave"),
	Island UMETA(DisplayName = "Island"),
	FloatingIsland UMETA(DisplayName = "Floating Island"),
	OpenWorld UMETA(DisplayName = "Open World")
};

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonVoxelSDFGPUParams
{
	GENERATED_BODY()

	UPROPERTY()
	int32 ModelType = 0;
};

UCLASS(Abstract, EditInlineNew, BlueprintType)
class DUNGEONARCHITECTRUNTIME_API UDungeonVoxelSDFModel : public UObject
{
	GENERATED_BODY()

public:
	virtual float GetBaseSDF(const FVector& WorldPos) const;
	virtual float CombinePrimarySDF(float BaseSDF, float ShapeSDF) const;
	virtual float CombineSecondarySDF(float CurrentSDF, float ShapeSDF, bool bAddOperation) const;
	virtual float ApplyWorldSpecificSDF(float CurrentSDF, const FVector& WorldPos) const { return CurrentSDF; }
	virtual void GetGPUParameters(FDungeonVoxelSDFGPUParams& OutParams) const;
	virtual EDungeonVoxelWorldType GetWorldType() const { return EDungeonVoxelWorldType::Cave; }
	virtual void Initialize(ADungeon* InDungeon) {}

	// Shape modification methods
	virtual void ModifyPrimaryShape(struct FDAVoxelShape& Shape) const {}

	// Get world-specific vertical offsets for volume placement
	virtual FDungeonVoxelVerticalOffsets GetVerticalOffsets() const { return FDungeonVoxelVerticalOffsets(); }

	// Get total ceiling height for volume placement (includes room height for caves, ground level for islands)
	virtual float GetTotalCeilingHeight(float BaseRoomHeight) const { return BaseRoomHeight; }

	// Post-processing for additional shapes (e.g., ceiling holes)
	virtual void PostProcessChunk(const struct FDungeonLayoutDataChunkInfo& ChunkInfo, const FBox& ChunkBounds,
	                              const FDungeonLayoutData& LayoutData, TArray<struct FDAVoxelShape>& OutAdditionalShapes, struct FRandomStream* RandomStream) const {}

	/**
	 * Get the bounds expansion needed for chunk searching based on world-specific features
	 * @param NoiseAmplitude The noise amplitude that affects shape boundaries
	 * @return The amount to expand bounds in each direction for chunk searching
	 */
	virtual FVector GetChunkSearchBoundsExpansion(float NoiseAmplitude) const
	{
		// Default: expand by noise amplitude to account for noise displacement
		return FVector(NoiseAmplitude, NoiseAmplitude, NoiseAmplitude);
	}

	/**
	 * Inject preprocessing commands for model-specific data generation
	 * @param Commands The command list to add preprocessing commands to
	 * @param Descriptor Shared pointer to the voxel chunk descriptor
	 */
	virtual void InjectPreprocessingCommands(FDungeonSceneProviderCommandList& Commands,
		TSharedPtr<FDAVoxelChunkDescriptor> Descriptor) const {}

	/**
	 * Called when applying world-specific SDF with access to preprocessed data
	 * @param CurrentSDF The current SDF value
	 * @param WorldPos The world position being evaluated
	 * @param Descriptor The descriptor containing cached model data
	 * @param ChunkCoord
	 * @param VoxelCoord
	 * @return The modified SDF value
	 */
	virtual float ApplyWorldSpecificSDFWithCache(float CurrentSDF, const FVector& WorldPos,
	                                             const FDAVoxelChunkDescriptor* Descriptor, const FIntVector& ChunkCoord, const FIntVector& VoxelCoord) const
	{
		return ApplyWorldSpecificSDF(CurrentSDF, WorldPos);
	}

	virtual FVector WarpBaseWorldPosition(const FVector& InWorldPos) { return InWorldPos; }
	virtual void WriteShaderParameters(FVoxelDensityComputeShader::FParameters* Parameters) {}
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonVoxelSDFModel_Cave : public UDungeonVoxelSDFModel
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Cave Settings", meta = (DisplayName = "Ceiling Extra Height"))
	float CeilingExtraHeight = 800.0f;

	UPROPERTY(EditAnywhere, Category = "Cave Settings", meta = (DisplayName = "Enable Ceiling Holes"))
	bool bEnableCeilingHoles = true;

	UPROPERTY(EditAnywhere, Category = "Cave Settings", meta = (DisplayName = "Ceiling Hole Radius", EditCondition = "bEnableCeilingHoles"))
	float CeilingHoleRadius = 600.0f;

	UPROPERTY(EditAnywhere, Category = "Cave Settings", meta = (DisplayName = "Ceiling Hole Height Offset", EditCondition = "bEnableCeilingHoles"))
	float CeilingHoleHeightOffset = 400.0f;

	UPROPERTY(EditAnywhere, Category = "Cave Settings", meta = (DisplayName = "Max Ceiling Holes Per Room", EditCondition = "bEnableCeilingHoles", UIMin = "0", UIMax = "10"))
	int32 MaxCeilingHolesPerRoom = 2;

	UPROPERTY(EditAnywhere, Category = "Cave Settings", meta = (DisplayName = "Ceiling Hole Spawn Probability", EditCondition = "bEnableCeilingHoles", UIMin = "0", UIMax = "1"))
	float CeilingHoleSpawnProbability = 0.5f;

public:
	virtual float GetBaseSDF(const FVector& WorldPos) const override;
	virtual float CombinePrimarySDF(float BaseSDF, float ShapeSDF) const override;
	virtual EDungeonVoxelWorldType GetWorldType() const override { return EDungeonVoxelWorldType::Cave; }

	// Cave-specific implementations
	virtual void ModifyPrimaryShape(FDAVoxelShape& Shape) const override;
	virtual FDungeonVoxelVerticalOffsets GetVerticalOffsets() const override;
	virtual float GetTotalCeilingHeight(float BaseRoomHeight) const override;
	virtual void PostProcessChunk(const FDungeonLayoutDataChunkInfo& ChunkInfo, const FBox& ChunkBounds,
	                              const FDungeonLayoutData& LayoutData, TArray<FDAVoxelShape>& OutAdditionalShapes, FRandomStream* RandomStream) const override;
	virtual void WriteShaderParameters(FVoxelDensityComputeShader::FParameters* Parameters) override;
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonVoxelSDFModel_Island : public UDungeonVoxelSDFModel
{
    GENERATED_BODY()

public:
	/** How deep the island platform extends below the ground level */
	UPROPERTY(EditAnywhere, Category = "Island Settings", meta = (DisplayName = "Platform Depth"))
	float PlatformDepth = 200.0f;

	/** Additional depth for variation at platform edges (creates more natural island look) */
	UPROPERTY(EditAnywhere, Category = "Island Settings", meta = (DisplayName = "Edge Taper Depth"))
	float EdgeTaperDepth = 0.0f;

	/** Water level height (Z position) */
	UPROPERTY(EditAnywhere, Category = "Water Settings", meta = (DisplayName = "Water Level"))
	float WaterLevel = 0.0f;

	/** How much the water slopes down away from islands (0 = flat, 0.1 = gentle slope) */
	UPROPERTY(EditAnywhere, Category = "Water Settings", meta = (DisplayName = "Shore Slope", UIMin = "0.0", UIMax = "1.0"))
	float ShoreSlope = 0.2f;

	/** Maximum horizontal distance the shoreline can extend from the island edge */
	UPROPERTY(EditAnywhere, Category = "Water Settings", meta = (DisplayName = "Shoreline Extent", UIMin = "0", UIMax = "10000"))
	float ShorelineExtent = 4000.0f;

	/** Falloff distance for shoreline effect to prevent discontinuities at chunk boundaries */
	UPROPERTY(EditAnywhere, Category = "Water Settings", meta = (DisplayName = "Shoreline Falloff Distance", UIMin = "100", UIMax = "5000"))
	float ShorelineFalloffDistance = 0.0f;

	/** Maximum depth below water level where shoreline effect is applied (prevents deep unnecessary geometry) */
	UPROPERTY(EditAnywhere, Category = "Water Settings", meta = (DisplayName = "Max Shore Depth", UIMin = "100", UIMax = "5000"))
	float MaxShoreDepth = 1000.0f;

	/** Hard cutoff - no shore effect beyond this depth below water level */
	UPROPERTY(EditAnywhere, Category = "Water Settings", meta = (DisplayName = "Shore Depth Cutoff", UIMin = "200", UIMax = "10000"))
	float ShoreDepthCutoff = 1000.0f;

public:
	virtual float GetBaseSDF(const FVector& WorldPos) const override;
	virtual float CombinePrimarySDF(float BaseSDF, float ShapeSDF) const override;
	virtual EDungeonVoxelWorldType GetWorldType() const override { return EDungeonVoxelWorldType::Island; }

	// Island-specific implementations
	virtual void ModifyPrimaryShape(FDAVoxelShape& Shape) const override;
	virtual FDungeonVoxelVerticalOffsets GetVerticalOffsets() const override;
	virtual float GetTotalCeilingHeight(float BaseRoomHeight) const override;
	virtual float ApplyWorldSpecificSDF(float CurrentSDF, const FVector& WorldPos) const override;
    virtual FVector GetChunkSearchBoundsExpansion(float NoiseAmplitude) const override;
	virtual void WriteShaderParameters(FVoxelDensityComputeShader::FParameters* Parameters) override;
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonVoxelSDFModel_FloatingIsland : public UDungeonVoxelSDFModel
{
    GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Island Settings")
	float PlatformDepth = 5000.0f;

	UPROPERTY(EditAnywhere, Category = "Island Settings")
	float SurfaceZ = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Island Settings")
    float ExpStrength = 2.0f;
    
    UPROPERTY(EditAnywhere, Category = "Island Settings")
    TObjectPtr<AActor> TipLocationActor;

	UPROPERTY(EditAnywhere, Category = "Island Settings")
	float CurlAnglePerMeter = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Island Settings")
	float ShapeSearchExpansionAmount = 2000;
	
public:
	virtual float GetBaseSDF(const FVector& WorldPos) const override;
	virtual float CombinePrimarySDF(float BaseSDF, float ShapeSDF) const override;
	virtual EDungeonVoxelWorldType GetWorldType() const override { return EDungeonVoxelWorldType::Island; }

	// Island-specific implementations
	virtual void ModifyPrimaryShape(FDAVoxelShape& Shape) const override;
	virtual FDungeonVoxelVerticalOffsets GetVerticalOffsets() const override;
	virtual float GetTotalCeilingHeight(float BaseRoomHeight) const override;
	virtual float ApplyWorldSpecificSDF(float CurrentSDF, const FVector& WorldPos) const override;
    virtual FVector GetChunkSearchBoundsExpansion(float NoiseAmplitude) const override;
	virtual FVector WarpBaseWorldPosition(const FVector& InWorldPos) override;
	virtual void WriteShaderParameters(FVoxelDensityComputeShader::FParameters* Parameters) override;

    virtual void Initialize(ADungeon* InDungeon) override;

private:
	FVector GetTipLocation() const;
	
private:
	FVector DefaultTipLocation = FVector::Zero();
};

