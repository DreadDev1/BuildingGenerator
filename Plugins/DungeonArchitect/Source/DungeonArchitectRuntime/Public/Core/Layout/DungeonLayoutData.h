//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Layout/FloorSettings.h"
#include "Core/Utils/DungeonPointOfInterest.h"
#include "Core/Utils/DungeonShapes.h"
#include "Frameworks/Canvas/DungeonCanvasRoomShapeTexture.h"
#include "DungeonLayoutData.generated.h"

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDungeonLayoutDataChunkInfo {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FDAShapeCircle> Circles;
    
    UPROPERTY()
    TArray<FDAShapePolygon> ConvexPolys;
    
    UPROPERTY()
    TArray<FDAShapeLine> Outlines;
    
    UPROPERTY()
    TArray<FDAShapeTexture> CanvasShapeTextures;

    UPROPERTY()
    TArray<FDungeonPointOfInterest> PointsOfInterest;
    
    void Append(const FDungeonLayoutDataChunkInfo& Other) {
        Circles.Append(Other.Circles);
        ConvexPolys.Append(Other.ConvexPolys);
        CanvasShapeTextures.Append(Other.CanvasShapeTextures);
        PointsOfInterest.Append(Other.PointsOfInterest);
        Outlines.Append(Other.Outlines);
    }
    
    void Append(const FDungeonLayoutDataChunkInfo& Other, const TArray<FName>& TagsToIgnore) {
        auto ShouldIgnore = [&TagsToIgnore](const TArray<FName>& ShapeTags) {
            for (const FName& IgnoreTag : TagsToIgnore) {
                if (ShapeTags.Contains(IgnoreTag)) {
                    return true;
                }
            }
            return false;
        };
        
        for (const FDAShapeCircle& OtherShape : Other.Circles) {
            if (!ShouldIgnore(OtherShape.Tags)) {
                Circles.Add(OtherShape);
            }
        }
        for (const FDAShapePolygon& OtherShape : Other.ConvexPolys) {
            if (!ShouldIgnore(OtherShape.Tags)) {
                ConvexPolys.Add(OtherShape);
            }
        }
        for (const FDAShapeTexture& OtherShape : Other.CanvasShapeTextures) {
            if (!ShouldIgnore(OtherShape.Tags)) {
                CanvasShapeTextures.Add(OtherShape);
            }
        }
        for (const FDAShapeLine& OtherShape : Other.Outlines) {
            if (!ShouldIgnore(OtherShape.Tags)) {
                Outlines.Add(OtherShape);
            }
        }

        // No need to filter points of interest by tags
        PointsOfInterest.Append(Other.PointsOfInterest);
    }
};

enum class FDungeonLayoutHeightRangeType : uint8 {
    EntireDungeon,
    Floor,
    CustomRange
};

class DUNGEONARCHITECTRUNTIME_API FDungeonLayoutHeightRange {
public:
    void SetHeightRangeForSingleFloor(const FDungeonFloorSettings& InFloorSettings, int32 InFloorIndex);
    void SetHeightRangeForMultipleFloors(const FDungeonFloorSettings& InFloorSettings, const TArray<int32>& InFloorIndices);
    void SetHeightRangeAcrossAllFloors();
    void SetHeightRangeCustom(float InHeightRangeMin, float InHeightRangeMax);
    void SetBaseHeightOffset(float InBaseHeightOffset) { BaseHeightOffset = InBaseHeightOffset; }
    int32 GetFloorIndexAtHeight(const FDungeonFloorSettings& InFloorSettings, float HeightZ) const;
    bool InsideActiveFloorHeightRange(float HeightZ) const;
		
    FDungeonLayoutHeightRangeType GetRangeType() const { return RangeType; }
    float GetMinHeight() const { return HeightRangeMin; }
    float GetMaxHeight() const { return HeightRangeMax; }
	
private:
    void UpdateFloorRange(const FDungeonFloorSettings& InFloorSettings);

private:
    // User vars
    FDungeonLayoutHeightRangeType RangeType = FDungeonLayoutHeightRangeType::EntireDungeon;
    TArray<int> ActiveFloorIndices;

    // Calculated values
    float HeightRangeMin = -100000000;
    float HeightRangeMax = 100000000;
    float BaseHeightOffset{};
};


USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDungeonLayoutDataDoorItem {
    GENERATED_BODY()
    
    UPROPERTY()
    FTransform WorldTransform = FTransform::Identity;

    UPROPERTY()
    float Width = 400;

    UPROPERTY()
    float Height = 400;
    
    /**
     * The layout is initially rendered without doors. Then the walls are removed to make way for doors, where ever it's needed.
     * This value controls how thick our wall removal brush is.
     * If you don't see your walls getting removed properly, try increasing it a bit
     */
    UPROPERTY()
    float DoorOcclusionThickness = 300;
    
    UPROPERTY()
    TArray<FVector> Outline_DEPRECATED;
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDungeonLayoutDataStairItem {
    GENERATED_BODY()
    
    UPROPERTY()
    FTransform WorldTransform = FTransform::Identity;

    UPROPERTY()
    float Width = 400;
};

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonLayoutData {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FDungeonLayoutDataChunkInfo> ChunkShapes;
    
    UPROPERTY()
    TArray<FDungeonLayoutDataDoorItem> Doors;
    
    UPROPERTY()
    TArray<FDungeonLayoutDataDoorItem> WallOpening;     // Not a door but no wall in this place

    UPROPERTY()
    TArray<FDungeonLayoutDataStairItem> Stairs;
    
    UPROPERTY()
    TArray<FDungeonPointOfInterest> PointsOfInterest;

    UPROPERTY()
    FDungeonFloorSettings FloorSettings;

    UPROPERTY(BlueprintReadOnly, Category="Dungeon Architect")
    FBox Bounds = FBox(ForceInitToZero);

    UPROPERTY(BlueprintReadOnly, Category="Dungeon Architect")
    FTransform DungeonTransform = FTransform::Identity;
};

class DUNGEONARCHITECTRUNTIME_API FDungeonLayoutUtils {
public:
    struct FCalcBoundsSettings {
        bool bIncludeTextureOverlays;
        FCalcBoundsSettings()
            : bIncludeTextureOverlays(false)
        {
        }
    };
    
    static FBox CalculateBounds(const FDungeonLayoutDataChunkInfo& LayoutShapes, const FCalcBoundsSettings& Settings = {});
    static FDungeonLayoutData FilterByHeightRange(const FDungeonLayoutData& InLayoutData, float MinHeight, float MaxHeight);
};


USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonLayoutDataSpatialCacheBucket {
    GENERATED_BODY()

    UPROPERTY()
    TArray<int32> ChunkIndices;
    
    UPROPERTY()
    TArray<int32> DoorIndices;
    
    UPROPERTY()
    TArray<int32> StairIndices;
};

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonLayoutDataSpatialCache {
    GENERATED_BODY()

    UPROPERTY()
    float BucketCellSize = 2000;

    UPROPERTY()
    TMap<FIntPoint, FDungeonLayoutDataSpatialCacheBucket> Buckets;
};

