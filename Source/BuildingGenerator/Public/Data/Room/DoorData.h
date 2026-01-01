// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/Generation/RoomGenerationTypes.h"
#include "Data/Grid/GridData.h"
#include "Engine/DataAsset.h"
#include "DoorData.generated.h"

// Forward declarations
struct FWallModule;
class ADoorway;



UCLASS()
class BUILDINGGENERATOR_API UDoorData : public UDataAsset
{
	GENERATED_BODY()

	public:
	// --- Door Frame Components (Static Geometry) ---
	
	// Door frame mesh (single mesh for complete frame)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frame Geometry")
	TSoftObjectPtr<UStaticMesh> FrameSideMesh;
		
	// Footprint in cells (2 = 200cm, 4 = 400cm))
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frame Geometry")
	int32 FrameFootprintY = 2;
	
	// Rotation offset for mesh alignment
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frame Geometry")
	FRotator FrameRotationOffset = FRotator::ZeroRotator;
	
	/* Position offsets for doors on North edge (+X boundary) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frame Geometry|Edge Offsets")
	FDoorPositionOffsets NorthEdgeOffsets;
    
	/* Position offsets for doors on South edge (-X boundary) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frame Geometry|Edge Offsets")
	FDoorPositionOffsets SouthEdgeOffsets;
    
	/* Position offsets for doors on East edge (+Y boundary) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frame Geometry|Edge Offsets")
	FDoorPositionOffsets EastEdgeOffsets;
    
	/* Position offsets for doors on West edge (-Y boundary) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frame Geometry|Edge Offsets")
	FDoorPositionOffsets WestEdgeOffsets;
	
	// --- Side Fill Configuration ---
    
	/* Strategy for filling gaps when door is smaller than standard doorway width */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Side Fills")
	EDoorwaySideFill SideFillType = EDoorwaySideFill::None;
    
	/* Wall modules to use for left side fill (only used if SideFillType = WallModules) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Side Fills", meta = (EditCondition = "SideFillType == EDoorwaySideFill::WallModules"))
	TArray<FWallModule> LeftSideModules;
    
	/* Wall modules to use for right side fill (only used if SideFillType = WallModules) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Side Fills", meta = (EditCondition = "SideFillType == EDoorwaySideFill::WallModules"))
	TArray<FWallModule> RightSideModules;
    
	/* Custom mesh for left side fill (only used if SideFillType = CustomMeshes) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Side Fills", meta = (EditCondition = "SideFillType == EDoorwaySideFill:: CustomMeshes"))
	TSoftObjectPtr<UStaticMesh> LeftSideMesh;
    
	/* Custom mesh for right side fill (only used if SideFillType = CustomMeshes) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Side Fills", meta = (EditCondition = "SideFillType == EDoorwaySideFill::CustomMeshes"))
	TSoftObjectPtr<UStaticMesh> RightSideMesh;
    
	/* Corner piece mesh (only used if SideFillType = CornerPieces) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Side Fills", meta = (EditCondition = "SideFillType == EDoorwaySideFill:: CornerPieces"))
	TSoftObjectPtr<UStaticMesh> CornerMesh;
	
	// --- Door Variety Pool (Hybrid System) ---
	
	// Door variety pool (for multiple door styles)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door Varieties")
	TArray<UDoorData*> DoorStylePool;
	
	// --- Functional Door Actor ---

	// The actual Blueprint Class of the Door Actor (e.g., an actor that handles opening/closing/replication)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door Functionality")
	TSubclassOf<ADoorway> DoorwayClass;

	// --- Connection Logic ---

	// Connection box extent (for hallway connections)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Connection")
	FVector ConnectionBoxExtent = FVector(50.0f, 50.0f, 200.0f);
	
	// Placement weight for this door style (if multiple are available)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Connection")
	float PlacementWeight = 1.0f;
	
	// ✅ NEW: Helper function to get offsets for specific edge
	/* Get position offsets for a specific wall edge */
	UFUNCTION(BlueprintPure, Category = "Door Data")
	FDoorPositionOffsets GetOffsetsForEdge(EWallEdge Edge) const
	{
		switch (Edge)
		{
		case EWallEdge::North:  return NorthEdgeOffsets;
		case EWallEdge::South: return SouthEdgeOffsets;
		case EWallEdge::East:   return EastEdgeOffsets;
		case EWallEdge::West:  return WestEdgeOffsets;
		default:                return FDoorPositionOffsets();
		}
	}
	
	/* Calculate total doorway width including frame and side fills
  * Side fills are always 1 cell each when present */
	UFUNCTION(BlueprintPure, Category = "Door Data")
	int32 GetTotalDoorwayWidth() const
	{
		int32 TotalWidth = FrameFootprintY;  // Start with frame (typically 2 cells)
        
		// Add side fills (1 cell each side when enabled)
		if (SideFillType != EDoorwaySideFill::None)
		{
			TotalWidth += 2;  // 1 left + 1 right = 2 cells
		}
        
		return TotalWidth;
	}
};
