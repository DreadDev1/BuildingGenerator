//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Voxel/Chunk/VoxelChunkDescriptor.h"

#include "Core/Layout/DungeonLayoutData.h"
#include "Frameworks/Voxel/SDFModels/VoxelSDFModel.h"

FDAVoxelShape FDAVoxelShape::CreateFromPolygon(const FDAShapePolygon& InPoly, float ExpansionAmount, EDAVoxelOperation Operation, EVoxelShapeLayer Layer) {
	FDAVoxelShape Shape;
	Shape.Type = EDAVoxelShapeType::ConvexPolygon;
	Shape.Transform = InPoly.Transform;
	Shape.Height = InPoly.Height;
	Shape.PolygonPoints = InPoly.Points;
	Shape.Operation = Operation;
	Shape.Layer = Layer;
	
	FVector2D Centroid = FVector2D::ZeroVector;
	for (const FVector2D& Point : Shape.PolygonPoints) {
		Centroid += Point;
	}
	if (Shape.PolygonPoints.Num() > 0) {
		Centroid /= Shape.PolygonPoints.Num();
		
		for (FVector2D& Point : Shape.PolygonPoints) {
			FVector2D Direction = Point - Centroid;
			if (Direction.Size() > 0.01f) {
				Direction.Normalize();
				Point += Direction * ExpansionAmount;
			}
		}
	}
	
	return Shape;
}

FDAVoxelShape FDAVoxelShape::CreateFromCircle(const FDAShapeCircle& InCircle, float ExpansionAmount, EDAVoxelOperation Operation, EVoxelShapeLayer Layer) {
	FDAVoxelShape Shape;
	Shape.Type = EDAVoxelShapeType::Circle;
	Shape.Transform = InCircle.Transform;
	Shape.Height = InCircle.Height;
	Shape.Radius = InCircle.Radius;
	Shape.Operation = Operation;
	Shape.Layer = Layer;
	
	Shape.Radius += ExpansionAmount;
	
	return Shape;
}

FDAVoxelShape FDAVoxelShape::CreateFromLine(const FDAShapeLine& InLine, float WallThickness, EDAVoxelOperation Operation, EVoxelShapeLayer Layer) {
	FDAVoxelShape Shape;
	Shape.Type = EDAVoxelShapeType::Wall;
	Shape.Transform = InLine.Transform;
	Shape.Height = InLine.Height;
	Shape.LineStart = InLine.LineStart;
	Shape.LineEnd = InLine.LineEnd;
	Shape.Thickness = WallThickness;
	Shape.Operation = Operation;
	Shape.Layer = Layer;
	
	return Shape;
}


FDAVoxelShape FDAVoxelShape::CreateCeilingHole(const FVector& Center, float HoleRadius, float MinHeight, EDAVoxelOperation Operation, EVoxelShapeLayer Layer) {
	FDAVoxelShape Shape;
	Shape.Type = EDAVoxelShapeType::CeilingHole;
	Shape.Transform = FTransform(Center);
	Shape.Radius = HoleRadius;
	Shape.HoleMinHeight = MinHeight;
	Shape.Operation = Operation;
	Shape.Layer = Layer;
	
	return Shape;
}

FBox FDAVoxelShape::GetWorldBounds() const {
	FBox Bounds(ForceInit);
	
	// The shape transform is at the base, height extends upward
	// CeilingHeightOffset adds additional height to the top
	if (Type == EDAVoxelShapeType::ConvexPolygon) {
		// Transform polygon points to world space and calculate bounds
		for (const FVector2D& Point2D : PolygonPoints) {
			FVector LocalPoint(Point2D.X, Point2D.Y, 0);
			FVector WorldPoint = Transform.TransformPosition(LocalPoint);
			Bounds += WorldPoint;
			// Height needs to be scaled by the transform's Z scale
			float ScaledHeight = Height * Transform.GetScale3D().Z;
			Bounds += WorldPoint + FVector(0, 0, ScaledHeight);
		}
	}
	else if (Type == EDAVoxelShapeType::Circle) {
		FVector Center = Transform.GetLocation();
		FVector RadiusVec(Radius, Radius, 0);
		Bounds += Center - RadiusVec;
		Bounds += Center + RadiusVec;
		// Height needs to be scaled by the transform's Z scale
		float ScaledHeight = Height * Transform.GetScale3D().Z;
		Bounds += Center + FVector(0, 0, ScaledHeight);
	}
	else if (Type == EDAVoxelShapeType::Wall) {
		// Transform line endpoints to world space
		FVector WorldStart = Transform.TransformPosition(FVector(LineStart.X, LineStart.Y, 0));
		FVector WorldEnd = Transform.TransformPosition(FVector(LineEnd.X, LineEnd.Y, 0));
		
		// Expand by thickness/2 in all horizontal directions
		float HalfThickness = Thickness * 0.5f;
		FVector ThicknessVec(HalfThickness, HalfThickness, 0);
		
		Bounds += WorldStart - ThicknessVec;
		Bounds += WorldStart + ThicknessVec;
		Bounds += WorldEnd - ThicknessVec;
		Bounds += WorldEnd + ThicknessVec;
		
		// Add height with ceiling offset - scale by transform's Z scale
		float ScaledHeight = Height * Transform.GetScale3D().Z;
		Bounds += WorldStart + FVector(0, 0, ScaledHeight);
		Bounds += WorldEnd + FVector(0, 0, ScaledHeight);
	}
	else if (Type == EDAVoxelShapeType::CeilingHole) {
		// Ceiling hole bounds - cylinder from MinHeight upward
		FVector Center = Transform.GetLocation();
		FVector RadiusVec(Radius, Radius, 0);
		
		// Horizontal bounds
		Bounds += Center - RadiusVec;
		Bounds += Center + RadiusVec;
		
		// Vertical bounds - from MinHeight to ceiling (don't extend infinitely for chunk calculation)
		Bounds += Center + FVector(0, 0, HoleMinHeight);
		Bounds += Center + FVector(0, 0, HoleMinHeight + 1000.0f); // Reasonable upper bound
	}
	
	return Bounds;
}

FIntVector FDAVoxelChunkDescriptor::WorldToChunkCoord(const FVector& WorldLocation) const {
	const float ChunkWorldSize = VoxelMeshSettings.VoxelChunkSize * VoxelMeshSettings.VoxelSize;
	return FIntVector(
		FMath::FloorToInt(WorldLocation.X / ChunkWorldSize),
		FMath::FloorToInt(WorldLocation.Y / ChunkWorldSize),
		FMath::FloorToInt(WorldLocation.Z / ChunkWorldSize)
	);
}

FBox FDAVoxelChunkDescriptor::GetChunkWorldBounds(const FIntVector& ChunkCoord) const {
	const float ChunkWorldSize = VoxelMeshSettings.VoxelChunkSize * VoxelMeshSettings.VoxelSize;
	const float Padding = VoxelMeshSettings.VoxelSize;
	
	FVector Min = FVector(ChunkCoord) * ChunkWorldSize - FVector(Padding);
	FVector Max = FVector(ChunkCoord + FIntVector(1)) * ChunkWorldSize + FVector(Padding);
	
	return FBox(Min, Max);
}

void FDAVoxelChunkDescriptor::Clear() {
	Shapes.Reset();
	Chunks.Reset();
	CustomDataChunks.Reset();
}

void FDAVoxelChunkDescriptor::RegisterShapeWithChunks(const FDAVoxelShape& InShape) {
	int32 ShapeIndex = Shapes.Add(InShape);

	// Get bounds expansion from SDF model
	BoundsExpansion = FVector(NoiseSettings.NoiseAmplitude, NoiseSettings.NoiseAmplitude, NoiseSettings.NoiseAmplitude);
	if (SDFModel) {
		BoundsExpansion = SDFModel->GetChunkSearchBoundsExpansion(NoiseSettings.NoiseAmplitude);
	}

	// Add some base padding for voxel resolution
	const float BasePadding = VoxelMeshSettings.VoxelSize * 2.0f;
	BoundsExpansion += FVector(BasePadding, BasePadding, BasePadding);

	FBox ShapeBounds = Shapes[ShapeIndex].GetWorldBounds();
	ShapeBounds = ShapeBounds.ExpandBy(BoundsExpansion);
		
	FIntVector MinChunk = WorldToChunkCoord(ShapeBounds.Min);
	FIntVector MaxChunk = WorldToChunkCoord(ShapeBounds.Max);
		
	for (int32 X = MinChunk.X; X <= MaxChunk.X; X++) {
		for (int32 Y = MinChunk.Y; Y <= MaxChunk.Y; Y++) {
			for (int32 Z = MinChunk.Z; Z <= MaxChunk.Z; Z++) {
				FIntVector ChunkCoord(X, Y, Z);
				FDAVoxelChunkInfo& Chunk = Chunks.FindOrAdd(ChunkCoord);
				Chunk.ChunkCoord = ChunkCoord;
				Chunk.ShapeIndices.AddUnique(ShapeIndex);
				Chunk.WorldBounds = GetChunkWorldBounds(ChunkCoord);
			}
		}
	}
}

void FDAVoxelChunkDescriptor::BuildFromLayoutData(const FDungeonLayoutData& LayoutData, const FTransform& InDungeonTransform, const TArray<FDAVoxelShape>& AdditionalShapes, FRandomStream* RandomStream) {
	Clear();

	const float ExpansionAmount = VoxelMeshSettings.VoxelSize * 0.5f;

	// Carve out the primary dungeon shapes
	for (const FDungeonLayoutDataChunkInfo& ChunkInfo : LayoutData.ChunkShapes) {
		FVector ChunkCenterSum = FVector::ZeroVector;
		int32 ChunkCenterSumCounter = 0;
		FBox ChunkBounds(ForceInit);
		
		for (const FDAShapePolygon& Poly : ChunkInfo.ConvexPolys) {
			FDAVoxelShape Shape = FDAVoxelShape::CreateFromPolygon(Poly, ExpansionAmount, EDAVoxelOperation::Subtract, EVoxelShapeLayer::PrimaryCarve);
			Shape.Transform = Shape.Transform * InDungeonTransform;;
			if (SDFModel) {
				SDFModel->ModifyPrimaryShape(Shape);
			}
			
			RegisterShapeWithChunks(Shape);
			
			if (Poly.Points.Num() > 0) {
				FVector2D MinPoint = Poly.Points[0];
				FVector2D MaxPoint = Poly.Points[0];
				for (const FVector2D& Point : Poly.Points) {
					MinPoint.X = FMath::Min(MinPoint.X, Point.X);
					MinPoint.Y = FMath::Min(MinPoint.Y, Point.Y);
					MaxPoint.X = FMath::Max(MaxPoint.X, Point.X);
					MaxPoint.Y = FMath::Max(MaxPoint.Y, Point.Y);

					// Accumulate the sum for the chunk center
					FVector WorldPoint = Poly.Transform.TransformPosition(FVector(Point.X, Point.Y, 0));
					ChunkCenterSum += WorldPoint;
					ChunkCenterSumCounter++;
					ChunkBounds += WorldPoint;
				}
			}
		}
		
		for (const FDAShapeCircle& Circle : ChunkInfo.Circles) {
			FDAVoxelShape Shape = FDAVoxelShape::CreateFromCircle(Circle, ExpansionAmount, EDAVoxelOperation::Subtract, EVoxelShapeLayer::PrimaryCarve);
			if (SDFModel) {
				SDFModel->ModifyPrimaryShape(Shape);
			}
			
			RegisterShapeWithChunks(Shape);

			FVector WorldCenter = Circle.Transform.GetLocation();
			
			int32 CountMult = 10;
			ChunkCenterSum += Circle.Transform.GetLocation() * CountMult;
			ChunkCenterSumCounter += CountMult;
			
			ChunkBounds += WorldCenter + FVector(Circle.Radius, 0, 0);
			ChunkBounds += WorldCenter - FVector(Circle.Radius, 0, 0);
			ChunkBounds += WorldCenter + FVector(0, Circle.Radius, 0);
			ChunkBounds += WorldCenter - FVector(0, Circle.Radius, 0);
		}

		// Apply SDF model post-processing (e.g., ceiling holes for caves)
		if (SDFModel && ChunkCenterSumCounter > 0) {
			TArray<FDAVoxelShape> PostProcessShapes;
			SDFModel->PostProcessChunk(ChunkInfo, ChunkBounds, LayoutData, PostProcessShapes, RandomStream);

			for (const FDAVoxelShape& Shape : PostProcessShapes) {
				RegisterShapeWithChunks(Shape);
			}
		}
	}
}

