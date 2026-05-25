//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Utils/DungeonShapes.h"
#include "Frameworks/Voxel/Chunk/VoxelChunkDataCPU.h"
#include "Frameworks/Voxel/Meshing/VoxelMeshGenerationSettings.h"
#include "VoxelChunkDescriptor.generated.h"

UENUM()
enum class EDAVoxelShapeType : uint8 {
	ConvexPolygon,
	Circle,
	Wall,
	CeilingHole
};

UENUM()
enum class EDAVoxelOperation : uint8 {
	Subtract,  // Carves out space (rooms, doors, holes)
	Add        // Adds solid geometry (walls, pillars)
};

UENUM()
enum class EVoxelShapeLayer : uint8 {
	PrimaryCarve = 0,
	AddGeometry = 1
};


// Voxel shape for SDF operations
USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDAVoxelShape {
	GENERATED_BODY()
	
	
	UPROPERTY()
	EDAVoxelShapeType Type = EDAVoxelShapeType::ConvexPolygon;
	
	UPROPERTY()
	FTransform Transform;
	
	UPROPERTY()
	float Height = 300.0f;
	
	// For ConvexPolygon type - 2D points forming the polygon base
	UPROPERTY()
	TArray<FVector2D> PolygonPoints;
	
	// For Circle and CeilingHole types
	UPROPERTY()
	float Radius = 0.0f;
	
	// For CeilingHole type - minimum height where hole starts carving upward
	UPROPERTY()
	float HoleMinHeight = 600.0f;
	
	// For elliptical holes - eccentricity (1.0 = circle, < 1.0 = ellipse)
	UPROPERTY()
	float Eccentricity = 1.0f;
	
	// For elliptical holes - rotation angle in radians
	UPROPERTY()
	float EllipseRotation = 0.0f;
	
	// For Wall type - line segment with thickness
	UPROPERTY()
	FVector2D LineStart = FVector2D::ZeroVector;
	
	UPROPERTY()
	FVector2D LineEnd = FVector2D::ZeroVector;
	
	UPROPERTY()
	float Thickness = 200.0f;  // Wall/door thickness
	
	// For Door type
	UPROPERTY()
	float Width = 400.0f;  // Door width
	
	UPROPERTY()
	float DoorOcclusionThickness = 300.0f;  // How thick the door carve-out is
	
	// Shape properties
	UPROPERTY()
	uint8 MaterialID = 0;

	UPROPERTY()
	int32 Priority = 0;
	
	UPROPERTY()
	EDAVoxelOperation Operation = EDAVoxelOperation::Subtract;  // CSG operation type
	
	UPROPERTY()
	EVoxelShapeLayer Layer = EVoxelShapeLayer::PrimaryCarve;  // Which CSG layer this shape belongs to

	UPROPERTY()
	bool bOverrideNoiseSettings = false;
	
	UPROPERTY()
	FDAVoxelNoiseSettings NoiseSettingsOverride;
	
	// Create from FDAShapePolygon
	static FDAVoxelShape CreateFromPolygon(const FDAShapePolygon& InPoly, float ExpansionAmount, EDAVoxelOperation Operation, EVoxelShapeLayer Layer);
	
	// Create from FDAShapeCircle
	static FDAVoxelShape CreateFromCircle(const FDAShapeCircle& InCircle, float ExpansionAmount, EDAVoxelOperation Operation, EVoxelShapeLayer Layer);
	
	// Create from FDAShapeLine (for walls)
	static FDAVoxelShape CreateFromLine(const FDAShapeLine& InLine, float WallThickness, EDAVoxelOperation Operation, EVoxelShapeLayer Layer);
	
	
	// Create a ceiling hole at specified location
	static FDAVoxelShape CreateCeilingHole(const FVector& Center, float HoleRadius, float MinHeight, EDAVoxelOperation Operation, EVoxelShapeLayer Layer);
	
	// Get world bounds of this shape
	FBox GetWorldBounds() const;
};

// Noise influence zone for localized noise control
USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDANoiseInfluenceZone {
	GENERATED_BODY()
	
	/** Transform of the influence zone (position, rotation, scale) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Influence Zone")
	FTransform Transform;
	
	UPROPERTY(EditAnywhere, Category="Voxel Volume")
	int32 Priority = 0;
	
	/** Inner box extent (half-size) where noise uses override settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Influence Zone")
	FVector InnerExtent = FVector(100.0f, 100.0f, 100.0f);
	
	/** Outer box extent (half-size) where noise blends back to global */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Influence Zone")
	FVector OuterExtent = FVector(200.0f, 200.0f, 200.0f);
	
	/** Use ellipsoid falloff instead of box */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Influence Zone")
	bool bUseEllipsoidFalloff = false;
	
	/** Enable noise amplitude override */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Overrides")
	bool bOverrideNoiseAmplitude = false;
	
	/** Override noise amplitude value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Overrides", meta=(EditCondition = "bOverrideNoiseAmplitude", UIMin = "0", UIMax = "500"))
	float OverrideNoiseAmplitude = 0.0f;
	
	/** Enable noise floor scale override */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Overrides")
	bool bOverrideNoiseFloorScale = false;
	
	/** Override noise floor scale value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Overrides", meta=(EditCondition = "bOverrideNoiseFloorScale", UIMin = "0", UIMax = "1"))
	float OverrideNoiseFloorScale = 1.0f;
	
	/** Enable noise scale override */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Overrides")
	bool bOverrideNoiseScale = false;
	
	/** Override noise scale value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Overrides", meta=(EditCondition = "bOverrideNoiseScale", UIMin = "10", UIMax = "1000"))
	float OverrideNoiseScale = 1.0f;
	
	/** Enable noise scale vector override */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Overrides")
	bool bOverrideNoiseScaleVector = false;
	
	/** Override noise scale vector value (directional stretching) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise Overrides", meta=(EditCondition = "bOverrideNoiseScaleVector"))
	FVector OverrideNoiseScaleVector = FVector(1.0f, 1.0f, 1.0f);
	
	/** Whether this zone is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Influence Zone")
	bool bEnabled = true;
};

// Chunk information for voxel generation
USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDAVoxelChunkInfo {
	GENERATED_BODY()
	
	UPROPERTY()
	FIntVector ChunkCoord = FIntVector::ZeroValue;
	
	UPROPERTY()
	TArray<int32> ShapeIndices;  // Indices into shared shape array
	
	UPROPERTY()
	FBox WorldBounds;
};

// Main descriptor for voxel chunk generation
USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDAVoxelChunkDescriptor {
	GENERATED_BODY()
	
	// All unique shapes across all chunks
	UPROPERTY()
	TArray<FDAVoxelShape> Shapes;
	
	// Chunk information map
	UPROPERTY()
	TMap<FIntVector, FDAVoxelChunkInfo> Chunks;
	
	// Voxel generation settings
	UPROPERTY()
	FDAVoxelMeshGenerationSettings VoxelMeshSettings;

	// Noise settings
	UPROPERTY()
	FDAVoxelNoiseSettings NoiseSettings;

	// SDF Model for world generation
	UPROPERTY()
	TObjectPtr<class UDungeonVoxelSDFModel> SDFModel;
	
	// Noise influence zones for localized noise control
	UPROPERTY()
	TArray<FDANoiseInfluenceZone> NoiseInfluenceZones;

	UPROPERTY()
	FVector BoundsExpansion = FVector::ZeroVector;
	
	// Generic cache for model-specific preprocessed data (e.g., Z=0 slices for islands)
	TMap<FName, FDAVoxelChunkDataCPU> CustomDataChunks;

	// Helper functions
	FIntVector WorldToChunkCoord(const FVector& WorldLocation) const;
	FBox GetChunkWorldBounds(const FIntVector& ChunkCoord) const;
	void Clear();

	// Build from dungeon layout data and additional shapes (e.g., volume actors)
	void BuildFromLayoutData(const struct FDungeonLayoutData& LayoutData, const FTransform& InDungeonTransform, const TArray<FDAVoxelShape>& AdditionalShapes = TArray<FDAVoxelShape>(), struct FRandomStream* RandomStream = nullptr);
	void RegisterShapeWithChunks(const FDAVoxelShape& InShape);

	// Type-safe access to cached model data
	FDAVoxelChunkDataCPU* GetOrCreateCustomDataChunk(const FName& Key, const TFunction<void(FDAVoxelChunkDataCPU&)>& Creator) {
		if (FDAVoxelChunkDataCPU* Chunk = CustomDataChunks.Find(Key)) {
			return Chunk;
		}
		
		FDAVoxelChunkDataCPU& NewChunk = CustomDataChunks.FindOrAdd(Key);
		Creator(NewChunk);
		return &NewChunk;
	}

	FDAVoxelChunkDataCPU* GetCustomDataChunk(const FName& Key) {
		return CustomDataChunks.Find(Key);
	}
	
	const FDAVoxelChunkDataCPU* GetCustomDataChunk(const FName& Key) const {
		return CustomDataChunks.Find(Key);
	}
};

