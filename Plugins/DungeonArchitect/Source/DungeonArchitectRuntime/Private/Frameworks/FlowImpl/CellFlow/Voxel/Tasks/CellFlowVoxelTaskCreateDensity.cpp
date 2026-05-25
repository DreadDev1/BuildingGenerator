//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/CellFlow/Voxel/Tasks/CellFlowVoxelTaskCreateDensity.h"

#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractNode.h"
#include "Frameworks/Flow/Domains/Voxel/FlowVoxelState.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutGraph.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Impl/CellFlowGrid.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowLib.h"
#include "Frameworks/Voxel/SDFModels/VoxelSDFModel.h"

bool UCellFlowVoxelTaskCreateDensity::CarveVoxels(DA::VDB::FVoxelGrid& VoxelGrid, const FFlowTaskExecutionSettings& InExecSettings, const FFlowExecNodeStatePtr& InState) const {
	// Mark that voxel generation is needed - actual generation happens in command execution
	if (UFlowVoxelState* VoxelState = InState->GetState<UFlowVoxelState>()) {
		VoxelState->bVoxelGenerationRequested = true;
		VoxelState->VoxelMeshSettings = VoxelMeshSettings;
		
		// Store noise parameters for later use
		VoxelState->VoxelMeshSettings.VoxelSize = VoxelMeshSettings.VoxelSize;
		
		UE_LOG(LogTemp, Warning, TEXT("Voxel generation requested - will be processed during command execution"));
	}
	
	// Don't generate density here - just mark for later processing
	return true;
}

void UCellFlowVoxelTaskCreateDensity::CarveGridLayout(DA::VDB::FVoxelGrid& VoxelGrid, UCellFlowLayoutGraph* InLayoutGraph, UDAFlowCellGraph* InCellGraph, const FVector& InGridSize) const {
	TMap<FGuid, const UFlowAbstractNode*> LayoutNodes;
	for (const UFlowAbstractNode* GraphNode : InLayoutGraph->GraphNodes) {
		const UFlowAbstractNode*& NodeRef = LayoutNodes.FindOrAdd(GraphNode->NodeId);
		NodeRef = GraphNode;
	}
	
	auto GetNoise = [this](const FVector& InLocation) -> float {
		if (DistortionDistance <= 0) {
			return 0.0f;
		}
		
		float Noise = 0;
		for (uint32 Octave = 0; Octave < NumOctaves; Octave++) {
			float OctaveShift = static_cast<float>(1 << Octave);
			float OctaveScale = OctaveShift / DistortionScale;
			Noise += FMath::PerlinNoise3D(InLocation * OctaveScale) / OctaveShift;
		}
		return Noise * DistortionDistance;
	};
	
	const float VoxelSize = VoxelMeshSettings.VoxelSize;
	const float NarrowBandWidth = VoxelSize * 3.0f;
	
	// Define room parameters in world units
	const float WallThicknessWorld = 100.0f; // Wall thickness in world units
	
	// Process each group node (represents a room/area)
	for (const FDAFlowCellGroupNode& GroupNode : InCellGraph->GroupNodes) {
		if (!GroupNode.IsActive()) continue;
		if (GroupNode.LeafNodes.Num() == 0) continue;
		
		// Check if the layout node is active
		const UFlowAbstractNode** LayoutNodePtr = LayoutNodes.Find(GroupNode.LayoutNodeID);
		const UFlowAbstractNode* LayoutNode = LayoutNodePtr ? *LayoutNodePtr : nullptr;
		if (!LayoutNode || !LayoutNode->bActive) {
			continue; // Skip inactive nodes
		}
		
		// Process each leaf node (grid cell) in the group
		for (const int LeafNodeId : GroupNode.LeafNodes) {
			if (const UDAFlowCellLeafNodeGrid* GridLeafNode = Cast<UDAFlowCellLeafNodeGrid>(InCellGraph->LeafNodes[LeafNodeId])) {
				if (GridLeafNode->LeafTileCoords.Num() == 0) {
					continue;
				}

				FIntPoint LocalMin, LocalMax;
				LocalMin = LocalMax = GridLeafNode->LeafTileCoords[0];
				for (const FIntPoint& LeafCoords : GridLeafNode->LeafTileCoords) {
					LocalMin.X = FMath::Min(LocalMin.X, LeafCoords.X);
					LocalMin.Y = FMath::Min(LocalMin.Y, LeafCoords.Y);

					LocalMax.X = FMath::Max(LocalMax.X, LeafCoords.X + 1);
					LocalMax.Y = FMath::Max(LocalMax.Y, LeafCoords.Y + 1);
				}
				// TODO Assumes a bounding box rather than treating them as individual tiles

				// Convert grid coordinates to world space
				// Add small expansion to ensure adjacent cells overlap properly
				const float CellExpansion = VoxelSize * 0.5f; // Half voxel expansion
				const FVector WorldMin = FVector(
					LocalMin.X * InGridSize.X - CellExpansion,
					LocalMin.Y * InGridSize.Y - CellExpansion,
					(GridLeafNode->LogicalZ * InGridSize.Z)  // Floor at logical Z
				);
				
				// For now, use default ceiling extra height
				// TODO: Get SDFModel from proper context
				float CeilingExtraHeight = 800.0f;
				const FVector WorldMax = FVector(
					LocalMax.X * InGridSize.X + CellExpansion,
					LocalMax.Y * InGridSize.Y + CellExpansion,
					(GridLeafNode->LogicalZ * InGridSize.Z) + CeilingExtraHeight
				);

				// Convert world space to voxel grid space
				// Use floor/ceil to ensure full coverage of the cell
				const FIntVector GridMin(
					FMath::FloorToInt(WorldMin.X / VoxelSize),
					FMath::FloorToInt(WorldMin.Y / VoxelSize),
					FMath::FloorToInt(WorldMin.Z / VoxelSize)
				);
				
				const FIntVector GridMax(
					FMath::CeilToInt(WorldMax.X / VoxelSize),
					FMath::CeilToInt(WorldMax.Y / VoxelSize),
					FMath::CeilToInt(WorldMax.Z / VoxelSize)
				);
				
				// Calculate world-space box parameters for SDF calculation
				// Use the actual world bounds, not the voxel-aligned bounds
				const FVector BoxCenter = (WorldMin + WorldMax) * 0.5f;
				const FVector BoxHalfExtents = (WorldMax - WorldMin) * 0.5f;
				
				// Expand iteration bounds for narrow band
				const int32 NarrowBandVoxels = FMath::CeilToInt(NarrowBandWidth / VoxelSize);
				const int32 NoiseThresholdVoxels = FMath::CeilToInt(DistortionDistance / VoxelSize);
				const FIntVector IterMin = GridMin - FIntVector(NarrowBandVoxels + NoiseThresholdVoxels);
				const FIntVector IterMax = GridMax + FIntVector(NarrowBandVoxels + NoiseThresholdVoxels);
				
				// Carve the box shape into the grid
				for (int x = IterMin.X; x <= IterMax.X; x++) {
					for (int y = IterMin.Y; y <= IterMax.Y; y++) {
						for (int z = IterMin.Z; z <= IterMax.Z; z++) {
							const FIntVector GridCoord(x, y, z);
							const FVector VoxelWorldPos = FVector(GridCoord) * VoxelSize;
							
							// Calculate box SDF (negative inside, positive outside)
							const FVector LocalPos = VoxelWorldPos - BoxCenter;
							const FVector AbsLocalPos(
								FMath::Abs(LocalPos.X),
								FMath::Abs(LocalPos.Y),
								FMath::Abs(LocalPos.Z)
							);
							
							// Box SDF calculation
							const FVector Q = AbsLocalPos - BoxHalfExtents;
							const float BoxSDF = FVector(FMath::Max(Q.X, 0.0f), FMath::Max(Q.Y, 0.0f), FMath::Max(Q.Z, 0.0f)).Size() 
								+ FMath::Min(FMath::Max(Q.X, FMath::Max(Q.Y, Q.Z)), 0.0f);

							const float Noise = GetNoise(VoxelWorldPos);
							
							// Invert for cave carving: positive inside rooms (air), negative outside (rock)
							// BoxSDF is negative inside, positive outside - we want the opposite
							const float CarveSDF = -BoxSDF; // -Noise;
							
							// Get existing voxel or use background
							DA::VDB::FVoxelData Voxel;
							DA::VDB::FVoxelData ExistingData;
							bool bHasExisting = VoxelGrid.Get(GridCoord, ExistingData);
							
							if (bHasExisting) {
								// Apply CSG subtraction (Max for cave carving) - take maximum to carve out space
								float OldSDF = ExistingData.SDF;
								Voxel.SDF = FMath::Max(ExistingData.SDF, CarveSDF);
								Voxel.Material = ExistingData.Material;
							}
							else {
								Voxel.SDF = CarveSDF;
								Voxel.Material = 0;
							}
							
							bool bShouldWrite = true; //(CarveSDF > 0.0f) || (FMath::Abs(Voxel.SDF) < NarrowBandWidth);
							
							if (bShouldWrite) {
								// Clamp to narrow band range
								//float PreClampSDF = Voxel.SDF;
								//Voxel.SDF = FMath::Clamp(Voxel.SDF, -NarrowBandWidth, NarrowBandWidth);
								
								bool bActive = FMath::Abs(Voxel.SDF) < NarrowBandWidth;
								VoxelGrid.Set(GridCoord, Voxel, bActive);
							}
						}
					}
				}
			}
		}
	}
}

void UCellFlowVoxelTaskCreateDensity::DebugCarveSphere(DA::VDB::FVoxelGrid& VoxelGrid) const {
	const float VoxelSize = VoxelMeshSettings.VoxelSize;
	
	// Write out a test density
	const FIntVector SphereGridCenter(0);
	constexpr int32 SphereExtent = 32;
	const FIntVector MinGrid(-SphereExtent);
	const FIntVector MaxGrid(SphereExtent);

	const FVector SphereWorldLocation = FVector(SphereGridCenter) * VoxelSize;
	const float SphereWorldRadius = SphereExtent * VoxelSize;
	const float NarrowBandWidth = VoxelSize * 3.0f;
	
	for (int x = MinGrid.X; x <= MaxGrid.X; x++) {
		for (int y = MinGrid.Y; y <= MaxGrid.Y; y++) {
			for (int z = MinGrid.Z; z <= MaxGrid.Z; z++) {
				const FIntVector GridCoord(x, y, z);
				const FVector VoxelWorldPos = FVector(GridCoord) * VoxelSize;
				const float Distance = FVector::Dist(VoxelWorldPos, SphereWorldLocation);
				const float SphereSDF = Distance - SphereWorldRadius;
				
				// Get existing voxel or use background
				DA::VDB::FVoxelData Voxel;
				{
					DA::VDB::FVoxelData ExistingData;
					if (VoxelGrid.Get(GridCoord, ExistingData)) {
						// Apply CSG ADD operation
						Voxel.SDF = FMath::Min(ExistingData.SDF, SphereSDF);
						Voxel.Material = ExistingData.Material;
					}
					else {
						Voxel.SDF = SphereSDF;
						Voxel.Material = 0;
					}
				}

				if (FMath::Abs(Voxel.SDF) < NarrowBandWidth) {
					VoxelGrid.Set(GridCoord, Voxel, true);
				}
			}
		}
	}
}

