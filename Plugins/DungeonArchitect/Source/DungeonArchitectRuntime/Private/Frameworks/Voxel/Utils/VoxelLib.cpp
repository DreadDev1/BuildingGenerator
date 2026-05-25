//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Voxel/Utils/VoxelLib.h"

#include "Frameworks/Voxel/Chunk/VoxelChunkDescriptor.h"
#include "Frameworks/Voxel/SDFModels/VoxelSDFModel.h"

struct FDAVoxelNoiseSettings;
const FName DA::FVoxelConstants::VoxelChunkActorTag = TEXT("da_voxel_mesh");
const float DA::FVoxelConstants::VoxelScaleMultiplierGPU = 1000;
const float DA::FVoxelConstants::VoxelScaleMultiplierCPU = 1000;

float DA::FVoxelLib::CalculateInfluenceZoneFactor(const FVector& WorldPos, const TArray<FDANoiseInfluenceZone>& InInfluenceZones,
	float& OutNoiseAmplitude, float& OutNoiseFloorScale, float& OutNoiseScaleNormalized, FVector& OutNoiseScaleVector)
{
	float TotalInfluence = 0.0f;
	float AccumAmplitude = 0.0f;
	float AccumFloorScale = 0.0f;
	float AccumScale = 0.0f;
	FVector AccumScaleVector = FVector::ZeroVector;
	bool bHasAmplitudeOverride = false;
	bool bHasFloorScaleOverride = false;
	bool bHasScaleOverride = false;
	bool bHasScaleVectorOverride = false;
	
	for (const FDANoiseInfluenceZone& Zone : InInfluenceZones) {
		if (!Zone.bEnabled) continue;
		
		// Transform world position to zone's local space
		FVector LocalPos = Zone.Transform.InverseTransformPosition(WorldPos);
		
		float Influence = 0.0f;
		
		if (Zone.bUseEllipsoidFalloff) {
			// Ellipsoid distance
			FVector NormPos = LocalPos / Zone.OuterExtent;
			float OuterDist = NormPos.Size();
			
			NormPos = LocalPos / Zone.InnerExtent;
			float InnerDist = NormPos.Size();
			
			if (InnerDist <= 1.0f) {
				Influence = 1.0f;
			} else if (OuterDist < 1.0f) {
				float T = (1.0f - InnerDist) / (OuterDist - InnerDist + 0.0001f);
				Influence = FMath::SmoothStep(0.0f, 1.0f, T);
			}
		} else {
			// Box distance
			FVector AbsPos = LocalPos.GetAbs();
			FVector InnerDist = (AbsPos - Zone.InnerExtent).ComponentMax(FVector::ZeroVector);
			FVector OuterDist = (AbsPos - Zone.OuterExtent).ComponentMax(FVector::ZeroVector);
			
			float InnerDistLength = InnerDist.Size();
			float OuterDistLength = OuterDist.Size();
			
			if (InnerDistLength <= 0.0f) {
				Influence = 1.0f;
			} else if (OuterDistLength <= 0.0f) {
				float T = 1.0f - (InnerDistLength / (Zone.OuterExtent - Zone.InnerExtent).Size());
				Influence = FMath::SmoothStep(0.0f, 1.0f, T);
			}
		}
		
		if (Influence > 0.0f) {
			TotalInfluence += Influence;
			
			// Accumulate override values weighted by influence
			if (Zone.bOverrideNoiseAmplitude) {
				AccumAmplitude += Zone.OverrideNoiseAmplitude * Influence;
				bHasAmplitudeOverride = true;
			}
			if (Zone.bOverrideNoiseFloorScale) {
				AccumFloorScale += Zone.OverrideNoiseFloorScale * Influence;
				bHasFloorScaleOverride = true;
			}
			if (Zone.bOverrideNoiseScale) {
				AccumScale += Zone.OverrideNoiseScale * Influence;
				bHasScaleOverride = true;
			}
			if (Zone.bOverrideNoiseScaleVector) {
				AccumScaleVector += Zone.OverrideNoiseScaleVector * Influence;
				bHasScaleVectorOverride = true;
			}
		}
	}
	
	// Output weighted averages if we have any overrides
	if (TotalInfluence > 0.0f) {
		if (bHasAmplitudeOverride) {
			OutNoiseAmplitude = AccumAmplitude / TotalInfluence;
		} else {
			OutNoiseAmplitude = -1.0f;
		}
		
		if (bHasFloorScaleOverride) {
			OutNoiseFloorScale = AccumFloorScale / TotalInfluence;
		} else {
			OutNoiseFloorScale = -1.0f;
		}
		
		if (bHasScaleOverride) {
			OutNoiseScaleNormalized = AccumScale / TotalInfluence;
		} else {
			OutNoiseScaleNormalized = -1.0f;
		}
		
		if (bHasScaleVectorOverride) {
			OutNoiseScaleVector = AccumScaleVector / TotalInfluence;
		} else {
			OutNoiseScaleVector = FVector(-1.0f, -1.0f, -1.0f);
		}
	} else {
		OutNoiseAmplitude = -1.0f;
		OutNoiseFloorScale = -1.0f;
		OutNoiseScaleNormalized = -1.0f;
		OutNoiseScaleVector = FVector(-1.0f, -1.0f, -1.0f);
	}
	
	return FMath::Min(TotalInfluence, 1.0f);
}

FVector DA::FVoxelLib::CalculateNoiseVector(const FVector& WorldPos, const FDAVoxelShape& InShape, 
	const FDAVoxelNoiseSettings& InGlobalNoiseSettings, const TArray<FDANoiseInfluenceZone>& InInfluenceZones)
{
	FDAVoxelNoiseSettings NoiseSettings = InShape.bOverrideNoiseSettings ? InShape.NoiseSettingsOverride : InGlobalNoiseSettings;
	// Apply influence zones
	{
		float InfluenceAmplitude = -1.0f;
		float InfluenceFloorScale = -1.0f;
		float InfluenceScaleNormalized = -1.0f;
		FVector InfluenceScaleVector = FVector(-1.0f, -1.0f, -1.0f);
		float InfluenceFactor = CalculateInfluenceZoneFactor(WorldPos, InInfluenceZones, InfluenceAmplitude, InfluenceFloorScale, InfluenceScaleNormalized, InfluenceScaleVector);
	
		if (InfluenceFactor > 0.0f) {
			// Blend between global and influence zone settings for enabled overrides
			if (InfluenceAmplitude >= 0.0f) {
				NoiseSettings.NoiseAmplitude = FMath::Lerp(NoiseSettings.NoiseAmplitude, InfluenceAmplitude, InfluenceFactor);
			}
			if (InfluenceFloorScale >= 0.0f) {
				NoiseSettings.NoiseFloorScale = FMath::Lerp(NoiseSettings.NoiseFloorScale, InfluenceFloorScale, InfluenceFactor);
			}
			if (InfluenceScaleNormalized >= 0.0f) {
				NoiseSettings.NoiseScaleNormalized = FMath::Lerp(NoiseSettings.NoiseScaleNormalized, InfluenceScaleNormalized, InfluenceFactor);
			}
			if (InfluenceScaleVector.X >= 0.0f) {
				NoiseSettings.NoiseScaleVector = FMath::Lerp(NoiseSettings.NoiseScaleVector, InfluenceScaleVector, InfluenceFactor);
			}
		}
	}

	FVector NoiseVector = CalculateNoiseVector(WorldPos, NoiseSettings);
	
	if (NoiseSettings.NoiseFloorScale != 1 || NoiseSettings.NoiseCeilingScale != 1)
	{
		const float NoiseFloorScale = InShape.Operation == EDAVoxelOperation::Subtract ? NoiseSettings.NoiseFloorScale : NoiseSettings.NoiseCeilingScale;
		const float NoiseCeilingScale = InShape.Operation == EDAVoxelOperation::Subtract ? NoiseSettings.NoiseCeilingScale : NoiseSettings.NoiseFloorScale;
		
		FVector LocalShapePos = InShape.Transform.InverseTransformPosition(WorldPos);
		float HeightLerp = FMath::SmoothStep<float>(0.0f, InShape.Height, LocalShapePos.Z);
		const float ZNoiseScale = FMath::Lerp(NoiseFloorScale, NoiseCeilingScale, HeightLerp);
		NoiseVector.Z *= ZNoiseScale;
	}

	return NoiseVector;
}

bool DA::FVoxelLib::InvertNoiseDisplacement_Picard(const FVector& P0, FVector& OutP1, const FDAVoxelNoiseSettings& NoiseSettings, int32 MaxIters, float Tol, float Alpha) {
	auto N = [&](const FVector& p) {
		return DA::FVoxelLib::CalculateNoiseVector(p, NoiseSettings);
	};

	// Better seed than P0: one predictor evaluation
	FVector x = P0 - N(P0);

	for (int32 i = 0; i < MaxIters; ++i) {
		const FVector r = x + N(x) - P0;      // F(x) = x + N(x) - P0
		if (r.Size() <= Tol) {
			OutP1 = x;
			return true;
		}

		const FVector xNext = P0 - N(x);      // Picard update
		x = FMath::Lerp(x, xNext, Alpha);     // damping
	}

	OutP1 = x;    // not fully converged, often still fine
	return false;
}

float DA::FVoxelLib::CalculateShapeSDF(const FDAVoxelShape& Shape, const FVector& InWorldPos, float InVoxelSize,
                                       const FDAVoxelNoiseSettings& InGlobalNoiseSettings, const TArray<FDANoiseInfluenceZone>& InInfluenceZones, uint8& OutMaterialID)
{
	FVector WorldPos = InWorldPos;
	FVector NoiseDisplacement = CalculateNoiseVector(WorldPos, Shape, InGlobalNoiseSettings, InInfluenceZones);
	WorldPos += NoiseDisplacement;
	
	// Now calculate the shape SDF with the already-displaced position
	// Transform world position to shape's local space
	const FVector LocalPos = Shape.Transform.InverseTransformPosition(WorldPos);
		
	// Early Z-bounds rejection for all shapes (except special cases)
	if (Shape.Type != EDAVoxelShapeType::CeilingHole) {
		const float ZBottom = 0.0f;
		const float ZTop = Shape.Height + 50.0f; // Add margin
		// Quick rejection if completely outside Z bounds
		if (LocalPos.Z < ZBottom - 100.0f || LocalPos.Z > ZTop + 100.0f) {
			return 10000.0f;
		}
	}
	
	float SDF = 10000.0f;
	OutMaterialID = 0;
		
	if (Shape.Type == EDAVoxelShapeType::ConvexPolygon) {
		// Early exit optimization: Check Z bounds first
		// Shape start at transform position (floor) and extends upward
		const float ZBottom = 0.0f;  // Floor is at the transform position
		const float ZTop = Shape.Height;
		const float ZBoxSDF = FMath::Max(ZBottom - LocalPos.Z, LocalPos.Z - ZTop);
			
		// Project to 2D (XY plane in local space)
		const FVector2D LocalPos2D(LocalPos.X, LocalPos.Y);
			
		// Phase 3.3: Fast path for axis-aligned rectangles (common case)
		const int32 NumPoints = Shape.PolygonPoints.Num();
			
		float PolygonSDF = 0.0f;
			
		if (NumPoints == 4) {
			// Check if it's an axis-aligned rectangle
			bool bIsAxisAligned = true;
			float MinX = 1e10f, MaxX = -1e10f, MinY = 1e10f, MaxY = -1e10f;
				
			for (int32 i = 0; i < 4; i++) {
				const FVector2D& P1 = Shape.PolygonPoints[i];
				const FVector2D& P2 = Shape.PolygonPoints[(i + 1) % 4];
					
				// Check if edge is axis-aligned
				if (FMath::Abs(P1.X - P2.X) > 0.01f && FMath::Abs(P1.Y - P2.Y) > 0.01f) {
					bIsAxisAligned = false;
					break;
				}
					
				MinX = FMath::Min(MinX, P1.X);
				MaxX = FMath::Max(MaxX, P1.X);
				MinY = FMath::Min(MinY, P1.Y);
				MaxY = FMath::Max(MaxY, P1.Y);
			}
				
			if (bIsAxisAligned) {
				// Fast 2D box SDF
				const float DX = FMath::Max(MinX - LocalPos2D.X, LocalPos2D.X - MaxX);
				const float DY = FMath::Max(MinY - LocalPos2D.Y, LocalPos2D.Y - MaxY);
				const FVector2D OutsideDist(FMath::Max(DX, 0.0f), FMath::Max(DY, 0.0f));
				const float InsideDist = FMath::Min(FMath::Max(DX, DY), 0.0f);
				PolygonSDF = OutsideDist.Size() + InsideDist;
			}
			else {
				// Fall back to general polygon SDF
				float MinDistSq = 1e10f;
				bool bInside = true;
					
				// Determine winding order by calculating polygon area
				float SignedArea = 0.0f;
				for (int32 i = 0; i < NumPoints; i++) {
					const FVector2D& P1 = Shape.PolygonPoints[i];
					const FVector2D& P2 = Shape.PolygonPoints[(i + 1) % NumPoints];
					SignedArea += (P1.X * P2.Y - P2.X * P1.Y);
				}
				const bool bCounterClockwise = SignedArea < 0.0f; // Negative area means CCW
					
				for (int32 i = 0; i < NumPoints; i++) {
					const FVector2D& P1 = Shape.PolygonPoints[i];
					const FVector2D& P2 = Shape.PolygonPoints[(i + 1) % NumPoints];
				
					// Edge vector and perpendicular
					const FVector2D Edge = P2 - P1;
					const FVector2D ToPoint = LocalPos2D - P1;
				
					// Check which side of edge we're on (for convex polygon)
					const float CrossProduct = Edge.X * ToPoint.Y - Edge.Y * ToPoint.X;
					// For CCW polygons: point is outside if cross product > 0
					// For CW polygons: point is outside if cross product < 0
					if ((bCounterClockwise && CrossProduct > 0) || (!bCounterClockwise && CrossProduct < 0)) {
						bInside = false;
					}
				
					// Calculate squared distance to edge segment
					const float EdgeLengthSq = Edge.X * Edge.X + Edge.Y * Edge.Y;
					if (EdgeLengthSq > 0.0001f) {
						const float T = FMath::Clamp((ToPoint.X * Edge.X + ToPoint.Y * Edge.Y) / EdgeLengthSq, 0.0f, 1.0f);
						const FVector2D Closest = P1 + T * Edge;
						const FVector2D Diff = LocalPos2D - Closest;
						const float DistSq = Diff.X * Diff.X + Diff.Y * Diff.Y;
						MinDistSq = FMath::Min(MinDistSq, DistSq);
					}
				}
					
				// Take square root only once
				float MinDist2D = FMath::Sqrt(MinDistSq);
					
				// Apply sign based on inside/outside
				PolygonSDF = bInside ? -MinDist2D : MinDist2D;
			}
		}
		else {
			// General convex polygon
			float MinDistSq = 1e10f;
			bool bInside = true;
				
			// Determine winding order by calculating polygon area
			float SignedArea = 0.0f;
			for (int32 i = 0; i < NumPoints; i++) {
				const FVector2D& P1 = Shape.PolygonPoints[i];
				const FVector2D& P2 = Shape.PolygonPoints[(i + 1) % NumPoints];
				SignedArea += (P1.X * P2.Y - P2.X * P1.Y);
			}
			const bool bCounterClockwise = SignedArea < 0.0f; // Negative area means CCW
				
			for (int32 i = 0; i < NumPoints; i++) {
				const FVector2D& P1 = Shape.PolygonPoints[i];
				const FVector2D& P2 = Shape.PolygonPoints[(i + 1) % NumPoints];
					
				// Edge vector and perpendicular
				const FVector2D Edge = P2 - P1;
				const FVector2D ToPoint = LocalPos2D - P1;
					
				// Check which side of edge we're on (for convex polygon)
				const float CrossProduct = Edge.X * ToPoint.Y - Edge.Y * ToPoint.X;
				// For CCW polygons: point is outside if cross product > 0
				// For CW polygons: point is outside if cross product < 0
				if ((bCounterClockwise && CrossProduct > 0) || (!bCounterClockwise && CrossProduct < 0)) {
					bInside = false;
				}
					
				// Calculate squared distance to edge segment
				const float EdgeLengthSq = Edge.X * Edge.X + Edge.Y * Edge.Y;
				if (EdgeLengthSq > 0.0001f) {
					const float T = FMath::Clamp((ToPoint.X * Edge.X + ToPoint.Y * Edge.Y) / EdgeLengthSq, 0.0f, 1.0f);
					const FVector2D Closest = P1 + T * Edge;
					const FVector2D Diff = LocalPos2D - Closest;
					const float DistSq = Diff.X * Diff.X + Diff.Y * Diff.Y;
					MinDistSq = FMath::Min(MinDistSq, DistSq);
				}
			}
				
			// Take square root only once
			float MinDist2D = FMath::Sqrt(MinDistSq);
				
			// Apply sign based on inside/outside
			PolygonSDF = bInside ? -MinDist2D : MinDist2D;
		}
			
		// Combine using max (intersection of polygon extrusion and Z bounds)
		SDF = FMath::Max(PolygonSDF, ZBoxSDF);
			
		// Determine material based on shape geometry - simpler approach
		if (SDF < 0) { // Inside the shape
			// Simple rule: if we're near the top or bottom AND inside the 2D projection, it's floor/ceiling
			// Otherwise it's a wall
				
			const float FloorBand = InVoxelSize * 2.0f; // Band near floor
			const float CeilingBand = InVoxelSize * 2.0f; // Band near ceiling
				
			// Check if we're in the floor or ceiling band
			bool bNearFloor = (LocalPos.Z >= ZBottom - FloorBand && LocalPos.Z <= ZBottom + FloorBand);
			bool bNearCeiling = (LocalPos.Z >= ZTop - CeilingBand && LocalPos.Z <= ZTop + CeilingBand);
				
			// Check if we're well inside the 2D polygon (not near edges)
			const float EdgeMargin = InVoxelSize * 2.0f;
			bool bInsidePolygon2D = (PolygonSDF < -EdgeMargin);
				
			// Material assignment with clear rules
			if (bNearFloor && bInsidePolygon2D) {
				OutMaterialID = 0; // Floor - only when near bottom AND inside 2D shape
			} else if (bNearCeiling && bInsidePolygon2D) {
				OutMaterialID = 2; // Ceiling - only when near top AND inside 2D shape
			} else {
				OutMaterialID = 1; // Wall - everything else (edges and mid-height)
			}
		}
	}
	else if (Shape.Type == EDAVoxelShapeType::Circle) {
		// Phase 3.2: Optimized cylinder SDF
		const float LocalXYDistSq = LocalPos.X * LocalPos.X + LocalPos.Y * LocalPos.Y;
		const float RadialDist = FMath::Sqrt(LocalXYDistSq) - Shape.Radius;
			
		// Calculate Z distance (1D box SDF in Z)
		// Shape starts at transform position (floor) and extends upward
		const float ZBottom = 0.0f;  // Floor is at the transform position
		const float ZTop = Shape.Height;
		const float ZBoxSDF = FMath::Max(ZBottom - LocalPos.Z, LocalPos.Z - ZTop);
			
		// Combine radial and Z distances (intersection)
		SDF = FMath::Max(RadialDist, ZBoxSDF);
			
		// Determine material based on shape geometry - simpler approach
		if (SDF < 0) { // Inside the shape
			const float FloorBand = InVoxelSize * 2.0f; // Band near floor
			const float CeilingBand = InVoxelSize * 2.0f; // Band near ceiling
				
			// Check if we're in the floor or ceiling band
			bool bNearFloor = (LocalPos.Z >= ZBottom - FloorBand && LocalPos.Z <= ZBottom + FloorBand);
			bool bNearCeiling = (LocalPos.Z >= ZTop - CeilingBand && LocalPos.Z <= ZTop + CeilingBand);
				
			// Check if we're well inside the circle (not near edge)
			const float EdgeMargin = InVoxelSize * 2.0f;
			bool bInsideCircle2D = (RadialDist < -EdgeMargin);
				
			// Material assignment with clear rules
			if (bNearFloor && bInsideCircle2D) {
				OutMaterialID = 0; // Floor - only when near bottom AND inside circle
			} else if (bNearCeiling && bInsideCircle2D) {
				OutMaterialID = 2; // Ceiling - only when near top AND inside circle
			} else {
				OutMaterialID = 1; // Wall - everything else (edges and mid-height)
			}
		}
	}
	else if (Shape.Type == EDAVoxelShapeType::CeilingHole) {
		float RadialDist;
			
		// Check if elliptical
		if (FMath::Abs(Shape.Eccentricity - 1.0f) > 0.01f) {
			// Elliptical hole with rotation
			float CosRot = FMath::Cos(Shape.EllipseRotation);
			float SinRot = FMath::Sin(Shape.EllipseRotation);
				
			// Rotate local position
			float RotX = LocalPos.X * CosRot - LocalPos.Y * SinRot;
			float RotY = LocalPos.X * SinRot + LocalPos.Y * CosRot;
				
			// Apply eccentricity (stretch along X axis)
			float EllipseX = RotX / Shape.Radius;
			float EllipseY = RotY / (Shape.Radius * Shape.Eccentricity);
				
			RadialDist = FMath::Sqrt(EllipseX * EllipseX + EllipseY * EllipseY) - 1.0f;
			RadialDist *= Shape.Radius; // Scale back to world units
		} else {
			// Simple circular hole (faster computation)
			const float DistSq = LocalPos.X * LocalPos.X + LocalPos.Y * LocalPos.Y;
			RadialDist = FMath::Sqrt(DistSq) - Shape.Radius;
		}
			
		// Only carve above the minimum height
		// Negative value carves upward, positive value keeps solid below
		const float HeightMask = Shape.HoleMinHeight - LocalPos.Z;
			
		// Combine: shape that only affects space above HoleMinHeight
		SDF = FMath::Max(RadialDist, HeightMask);
			
		// Material assignment for ceiling holes
		if (SDF < 0) { // Inside the ceiling hole shape
			// The hole carves through the ceiling, so surfaces near the hole edge
			// should use the ceiling hole material (ID 3)
			OutMaterialID = 3; // Ceiling hole material
		}
	}
	else if (Shape.Type == EDAVoxelShapeType::Wall) {
		// Wall SDF - creates solid geometry (not carved out)
		const FVector2D LocalPos2D(LocalPos.X, LocalPos.Y);
			
		// Calculate distance to line segment
		const FVector2D LineDir = Shape.LineEnd - Shape.LineStart;
		const float LineLength = LineDir.Size();
			
		float Dist2D = 0.0f;
		if (LineLength > 0.01f) {
			const FVector2D LineNorm = LineDir / LineLength;
			const FVector2D ToPoint = LocalPos2D - Shape.LineStart;
			const float T = FMath::Clamp(FVector2D::DotProduct(ToPoint, LineNorm), 0.0f, LineLength);
			const FVector2D Closest = Shape.LineStart + LineNorm * T;
			// Distance to the center line of the wall
			Dist2D = (LocalPos2D - Closest).Size();
		}
		else {
			// Degenerate line - treat as point
			Dist2D = LocalPos2D.Size();
		}
			
		// Create a box-like SDF for the wall
		// The wall extends thickness/2 on each side of the line
		const float WallSDF2D = Dist2D - Shape.Thickness * 0.5f;
			
		// Calculate Z distance - walls should match room height convention
		// Shape starts at transform position (floor) and extends upward
		const float ZBottom = 0.0f;  // Floor is at the transform position
		const float ZTop = Shape.Height;
		const float ZBoxSDF = FMath::Max(ZBottom - LocalPos.Z, LocalPos.Z - ZTop);
			
		// Combine to create wall volume
		// Use max to create intersection of the extruded wall shape and Z bounds
		SDF = FMath::Max(WallSDF2D, ZBoxSDF);
	}
		
	return SDF;
}

FVector DA::FVoxelLib::CalculateNoiseVector(const FVector& WorldPos, const FDAVoxelNoiseSettings& NoiseSettings) {
	if (NoiseSettings.NoiseAmplitude <= 0) {
		return FVector::Zero();
	}
	
	const float Separation = NoiseSettings.NoiseChannelSeparation;
	const FVector BaseOffset = NoiseSettings.NoiseOffset;
	const float NoiseScale = NoiseSettings.GetNoiseScale(false);
	FVector SampleLocation = WorldPos + BaseOffset;
	const int32 NumOctaves = NoiseSettings.NoiseOctaves;
	
	// Apply domain warping using IQ's nested FBM formula
	if (NoiseSettings.bEnableDomainWarp && NoiseSettings.DomainWarpStrength > 0.0f) {
		// Helper lambda for simple FBM
		auto SimpleFBM = [NumOctaves, NoiseScale](const FVector& Position) -> FVector {
			FVector Result = FVector::ZeroVector;
			float Amp = 1.0f;
			float Freq = 1.0f;
			const FVector Pos = Position / NoiseScale;
			
			for (int32 i = 0; i < NumOctaves; i++) {
				// X component with offset for decorrelation
				Result.X += FMath::PerlinNoise3D(Pos * Freq + FVector(0, 0, 0)) * Amp;
				// Y component with offset for decorrelation
				Result.Y += FMath::PerlinNoise3D(Pos * Freq + FVector(123.456, 234.567, 345.678)) * Amp;
				// Z component with offset for decorrelation
				Result.Z += FMath::PerlinNoise3D(Pos * Freq + FVector(456.789, 567.890, 678.901)) * Amp;
				
				Amp *= 0.5f;
				Freq *= 2.0f;
			}
			
			return Result;
		};
		
		// First FBM to calculate the warp offset (IQ's formula)
		FVector WarpOffset = SimpleFBM(SampleLocation);
		
		// Apply the warp with strength control
		// The warp is scaled by both the domain warp strength and the scale
		SampleLocation += WarpOffset * NoiseSettings.DomainWarpStrength * NoiseScale;
	}
	
	auto Fbm = [NumOctaves](const FVector& InLocation) {
		float Noise = 0;
		float Amplitude = 1;
		float Frequency = 1;

		for (int32 Octave = 0; Octave < NumOctaves; Octave++) {
			const FVector NoisePos = (InLocation) * Frequency;
			Noise += FMath::PerlinNoise3D(NoisePos) * Amplitude;
	
			Amplitude *= 0.5f;
			Frequency *= 2;
		}
		return Noise;
	};
	
	
	FVector NoiseVector;
	NoiseVector.X = Fbm((SampleLocation + FVector(Separation, 0, 0)) / NoiseScale);
	NoiseVector.Y = Fbm((SampleLocation + FVector(0, Separation, 0)) / NoiseScale);
	NoiseVector.Z = Fbm((SampleLocation + FVector(0, 0, Separation)) / NoiseScale);

	// Apply amplitude and Z-axis scaling
	NoiseVector *= NoiseSettings.NoiseAmplitude;

	// Apply directional scaling to create elongated features
	NoiseVector *= NoiseSettings.NoiseScaleVector;

	return NoiseVector;
}

bool DA::FVoxelMathUtils::IsPointInPolygon(const FVector2D& Point, const TArray<FVector2D>& Polygon) {
	int32 CrossingCount = 0;
	
	for (int32 i = 0; i < Polygon.Num(); i++) {
		const FVector2D& P1 = Polygon[i];
		const FVector2D& P2 = Polygon[(i + 1) % Polygon.Num()];
		
		// Check if ray from point to +X crosses this edge
		if ((P1.Y <= Point.Y && P2.Y > Point.Y) || (P1.Y > Point.Y && P2.Y <= Point.Y)) {
			float T = (Point.Y - P1.Y) / (P2.Y - P1.Y);
			float XIntersect = P1.X + T * (P2.X - P1.X);
			
			if (Point.X < XIntersect) {
				CrossingCount++;
			}
		}
	}
	
	return (CrossingCount % 2) == 1;
}

float DA::FVoxelMathUtils::DistanceToLineSegment2D(const FVector2D& Point, const FVector2D& A, const FVector2D& B) {
	const FVector2D AB = B - A;
	const FVector2D AP = Point - A;
	
	const float T = FMath::Clamp(FVector2D::DotProduct(AP, AB) / FVector2D::DotProduct(AB, AB), 0.0f, 1.0f);
	const FVector2D Closest = A + T * AB;
	
	return FVector2D::Distance(Point, Closest);
}

////////////////////////// FVoxelLibSSE //////////////////////////
VectorRegister DA::FVoxelLibSSE::CalculateSphereDistanceVectorized(const VectorRegister& VecPosX, const VectorRegister& VecPosY, const VectorRegister& VecPosZ, const FVector& Center, float Radius) {
	VectorRegister VecCenterX = VectorSetFloat1(Center.X);
	VectorRegister VecCenterY = VectorSetFloat1(Center.Y);
	VectorRegister VecCenterZ = VectorSetFloat1(Center.Z);
	VectorRegister VecRadius = VectorSetFloat1(Radius);
		
	// Distance from center
	VectorRegister DX = VectorSubtract(VecPosX, VecCenterX);
	VectorRegister DY = VectorSubtract(VecPosY, VecCenterY);
	VectorRegister DZ = VectorSubtract(VecPosZ, VecCenterZ);
		
	// Squared distance
	VectorRegister DistSq = VectorAdd(VectorAdd(VectorMultiply(DX, DX), VectorMultiply(DY, DY)), VectorMultiply(DZ, DZ));
		
	// Approximate sqrt using reciprocal sqrt
	VectorRegister InvDist = VectorReciprocalSqrt(VectorAdd(DistSq, VectorSetFloat1(0.0001f)));
	VectorRegister Dist = VectorDivide(VectorOne(), InvDist);
		
	// SDF = distance - radius
	return VectorSubtract(Dist, VecRadius);
}

float DA::FVoxelLib::CalculateBaseSDF(const FVector& InWorldPos, const TArray<FDAVoxelShape>& InShapes,
                                      float InVoxelSize, const FDAVoxelNoiseSettings& InGlobalNoiseSettings,
                                      const TArray<FDANoiseInfluenceZone>& InInfluenceZones, UDungeonVoxelSDFModel* SDFModel,
                                      const FDAVoxelChunkDescriptor* Descriptor, const FIntVector& InChunkCoord, const FBox& ChunkExpandedBounds, const FIntVector& VoxelCoord, uint8& OutMaterialID)
{
	if (!SDFModel) {
		return -10000.0f;
	}

	FVector WorldPos = SDFModel->WarpBaseWorldPosition(InWorldPos);
	//if (!ChunkExpandedBounds.IsInsideOrOn(WorldPos)) {
	//	return SDFModel->GetBaseSDF(WorldPos);
	//}

	TArray<FDAVoxelShape> PrimaryCarveShapes;
	TArray<FDAVoxelShape> AddGeometryShapes;

	for (const FDAVoxelShape& Shape : InShapes) {
		switch (Shape.Layer) {
		case EVoxelShapeLayer::PrimaryCarve:
			PrimaryCarveShapes.Add(Shape);
			break;
		case EVoxelShapeLayer::AddGeometry:
			AddGeometryShapes.Add(Shape);
			break;
		}
	}

	float SDF = SDFModel->GetBaseSDF(WorldPos);
	OutMaterialID = 0;

	float ClosestShapeDist = 10000.0f;
	uint8 ClosestShapeMaterial = 0;

	for (const FDAVoxelShape& Shape : PrimaryCarveShapes) {
		uint8 ShapeMaterial = 0;
		float ShapeSDF = DA::FVoxelLib::CalculateShapeSDF(Shape, WorldPos, InVoxelSize,
			InGlobalNoiseSettings, InInfluenceZones, ShapeMaterial);

		// Let the model decide how to combine - it knows whether to carve or add
		SDF = SDFModel->CombinePrimarySDF(SDF, ShapeSDF);

		if (ShapeSDF < 0 && FMath::Abs(ShapeSDF) < FMath::Abs(ClosestShapeDist)) {
			ClosestShapeDist = ShapeSDF;
			ClosestShapeMaterial = ShapeMaterial;
		}
	}

	if (ClosestShapeDist < 0) {
		OutMaterialID = ClosestShapeMaterial;
	}

	// Use the new cached method when descriptor is available
	SDF = SDFModel->ApplyWorldSpecificSDFWithCache(SDF, WorldPos, Descriptor, InChunkCoord, VoxelCoord);

	for (const FDAVoxelShape& Shape : AddGeometryShapes) {
		uint8 ShapeMaterial = 0;
		float ShapeSDF = DA::FVoxelLib::CalculateShapeSDF(Shape, WorldPos, InVoxelSize,
			InGlobalNoiseSettings, InInfluenceZones, ShapeMaterial);
		SDF = SDFModel->CombineSecondarySDF(SDF, ShapeSDF, Shape.Operation == EDAVoxelOperation::Add);
	}
	
	return SDF;
}

