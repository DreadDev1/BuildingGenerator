//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "ProceduralMarkerEmitter.generated.h"

class ADungeon;
class UDungeonBuilder;

UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable, abstract, HideDropDown)
class DUNGEONARCHITECTRUNTIME_API UProceduralMarkerEmitter : public UObject {
	GENERATED_BODY()
	
public:
	/**
	 * If the builder doesn't intrinsically support this proc marker emitter, this function would
	 * be called, giving the author an opportunity to support it themselves
	 */
	virtual void HandleUnsupportedBuilder(ADungeon* InDungeon) const {}
};

UENUM()
enum class EProcMarkerGenSimpleHeightFunction {
	LowestPoint,
	HighestPoint,
};

UENUM()
enum class EProcMarkerGenHeightFunction {
	OriginalHeight,
	LowestPoint,
	HighestPoint,
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UBoxBoundaryMarkerEmitter : public UProceduralMarkerEmitter {
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Box Boundary", meta=(UIMin=1))
	int SizeMultiplier = 1;

	/**
	 * No. of tiles to pad on the boundary. This is in local coords and not world coord.  This value would be multiplied by the grid size 
	 */
	UPROPERTY(EditAnywhere, Category="Box Boundary", meta=(UIMin=1))
	int TilePadding = 0;

	/** The boundary box might be bigger than the actual boundary (if the size multiplier is higher or the tile padding is non-zero. In this case, align the box to the center */
	UPROPERTY(EditAnywhere, Category="Box Boundary", meta=(UIMin=1))
	bool bAlignToCenter{true};

	UPROPERTY(EditAnywhere, Category="Box Boundary")
	EProcMarkerGenSimpleHeightFunction HeightFunction = EProcMarkerGenSimpleHeightFunction::LowestPoint;
	
	UPROPERTY(EditAnywhere, Category="Box Boundary")
	bool bEmitEdges{true};

	UPROPERTY(EditAnywhere, Category="Box Boundary", meta=(EditCondition="bEmitEdges"))
	FString EdgeMarkerName = "BoxBoundaryWall";
	
	UPROPERTY(EditAnywhere, Category="Box Boundary")
	bool bEmitEdgeSeparators{true};
	
	UPROPERTY(EditAnywhere, Category="Box Boundary", meta=(EditCondition="bEmitEdgeSeparators"))
	FString EdgeSeparatorMarkerName = "BoxBoundaryWallSeparator";
	
	UPROPERTY(EditAnywhere, Category="Box Boundary")
	bool bEmitCorners{};
	
	UPROPERTY(EditAnywhere, Category="Box Boundary", meta=(EditCondition="bEmitCorners"))
	FString CornerMarkerName = "BoxBoundaryCorner";
	
	UPROPERTY(EditAnywhere, Category="Box Boundary")
	bool bEmitGroundTiles{};
	
	UPROPERTY(EditAnywhere, Category="Box Boundary", meta=(EditCondition="bEmitGroundTiles"))
	FString GroundMarkerName = "BoxBoundaryGround";

public:
	void EmitMarkers(const FVector& GridSize, const FIntVector& CoordMin, const FIntVector& CoordMax, const FTransform& DungeonTransform,
				const TFunction<void(const FString&, const FTransform&)> FnEmitMarker) const;
};


/*
UCLASS()
class DUNGEONARCHITECTRUNTIME_API UBoundaryMarkerEmitter : public UProceduralMarkerEmitter {
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Base Names")
	TArray<FString> TileMarkerNames { { "Ground" } };

	UPROPERTY(EditAnywhere, Category = "Base Names")
	TArray<FString> EdgeMarkerNames { { "Wall", "Fence" } };
 
	UPROPERTY(EditAnywhere, Category="Boundary")
	bool bEmitEdges{};

	UPROPERTY(EditAnywhere, Category="Boundary", meta=(EditCondition="bEmitEdges"))
	FString EdgeMarkerName = "BoundaryWall";
	
	UPROPERTY(EditAnywhere, Category="Boundary")
	bool bEmitCorners{};
	
	UPROPERTY(EditAnywhere, Category="Boundary", meta=(EditCondition="bEmitCorners"))
	FString CornerMarkerName = "BoundaryCorner";
	
	UPROPERTY(EditAnywhere, Category="Boundary")
	bool bEmitGroundTiles{};
	
	UPROPERTY(EditAnywhere, Category="Boundary", meta=(EditCondition="bEmitGroundTiles"))
	FString GroundMarkerName = "BoundaryGround";

public:
	bool Execute(IMarkerGenProcessor& InProcessor, const FRandomStream& InRandom, TArray<FDAMarkerInfo>& InOutMarkerList);
};
*/

