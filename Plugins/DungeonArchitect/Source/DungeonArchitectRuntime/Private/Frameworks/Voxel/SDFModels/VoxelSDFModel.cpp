//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Voxel/SDFModels/VoxelSDFModel.h"

#include "Core/Layout/DungeonLayoutData.h"
#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommandVoxel.h"
#include "Frameworks/Voxel/Chunk/VoxelChunkDataCPU.h"
#include "Frameworks/Voxel/Chunk/VoxelChunkDescriptor.h"

float UDungeonVoxelSDFModel::GetBaseSDF(const FVector& WorldPos) const
{
	// Default: solid world (negative = solid in standard SDF)
	return -10000.0f;
}

float UDungeonVoxelSDFModel::CombinePrimarySDF(float BaseSDF, float ShapeSDF) const
{
	// Default carving operation (make shapes empty)
	// ShapeSDF is negative inside, we want those areas positive (empty) for carving
	return FMath::Max(BaseSDF, -ShapeSDF);
}

float UDungeonVoxelSDFModel::CombineSecondarySDF(float CurrentSDF, float ShapeSDF, bool bAddOperation) const
{
	// SDF negative = solid, positive = empty
	// Add: Make area solid (negative) - use Min to add solid
	// Subtract: Make area empty (positive) - use Max to carve out
	if (bAddOperation) {
		// Add solid geometry - shape is negative inside, use Min
		return FMath::Min(CurrentSDF, ShapeSDF);
	} else {
		// Subtract/carve - invert shape SDF and use Max
		return FMath::Max(CurrentSDF, -ShapeSDF);
	}
}

void UDungeonVoxelSDFModel::GetGPUParameters(FDungeonVoxelSDFGPUParams& OutParams) const
{
	OutParams = FDungeonVoxelSDFGPUParams();
	OutParams.ModelType = (int32)GetWorldType();
}

float UDungeonVoxelSDFModel_Cave::GetBaseSDF(const FVector& WorldPos) const
{
	// Caves start with solid rock (negative = solid)
	return -10000.0f;
}

float UDungeonVoxelSDFModel_Cave::CombinePrimarySDF(float BaseSDF, float ShapeSDF) const
{
	// Carve out rooms (shapes become empty space)
	// ShapeSDF is negative inside rooms, but we want those areas to be positive (empty)
	// So we invert the shape SDF and take max
	return FMath::Max(BaseSDF, -ShapeSDF);
}

void UDungeonVoxelSDFModel_Cave::ModifyPrimaryShape(FDAVoxelShape& Shape) const
{
	// Add extra ceiling height for cave environments
	Shape.Height += CeilingExtraHeight;
}

FDungeonVoxelVerticalOffsets UDungeonVoxelSDFModel_Cave::GetVerticalOffsets() const
{
	FDungeonVoxelVerticalOffsets Offsets;
	Offsets.TopOffset = CeilingExtraHeight;  // Caves need extra ceiling space
	Offsets.BottomOffset = 0.0f;
	return Offsets;
}

float UDungeonVoxelSDFModel_Cave::GetTotalCeilingHeight(float BaseRoomHeight) const
{
	// For caves: room height + extra ceiling height
	return BaseRoomHeight + CeilingExtraHeight;
}

void UDungeonVoxelSDFModel_Cave::PostProcessChunk(const FDungeonLayoutDataChunkInfo& ChunkInfo, const FBox& ChunkBounds,
	const FDungeonLayoutData& LayoutData, TArray<FDAVoxelShape>& OutAdditionalShapes, FRandomStream* RandomStream) const
{
	if (!bEnableCeilingHoles) {
		return;
	}

	float ChunkDiameter = ChunkBounds.GetExtent().GetMax();
	FVector ChunkCenter = ChunkBounds.GetCenter();

	if (ChunkDiameter < 1500.0f) {  // Min room size threshold,. TODO: make me parameterized
		return;
	}

	float RandomValue = RandomStream ? RandomStream->FRand() : FMath::FRand();
	if (RandomValue > CeilingHoleSpawnProbability) {
		return;
	}

	// Calculate hole start height
	float BaseRoomHeight = ChunkInfo.ConvexPolys.Num() > 0 ? ChunkInfo.ConvexPolys[0].Height :
						   (ChunkInfo.Circles.Num() > 0 ? ChunkInfo.Circles[0].Height : 800.0f);
	float TotalRoomHeight = BaseRoomHeight + CeilingExtraHeight;
	float HoleMinHeight = TotalRoomHeight - CeilingHoleHeightOffset;

	int32 NumHolesToCreate = FMath::Min(MaxCeilingHolesPerRoom, FMath::Max(1, MaxCeilingHolesPerRoom));

	for (int32 HoleIdx = 0; HoleIdx < NumHolesToCreate; HoleIdx++) {
		FVector HolePosition = ChunkCenter;

		// Randomize position for multiple holes
		if (NumHolesToCreate > 1) {
			float Angle = (RandomStream ? RandomStream->FRand() : FMath::FRand()) * 2.0f * PI;
			float RadiusOffset = (RandomStream ? RandomStream->FRand() : FMath::FRand()) * ChunkDiameter * 0.3f;
			HolePosition.X += FMath::Cos(Angle) * RadiusOffset;
			HolePosition.Y += FMath::Sin(Angle) * RadiusOffset;
		}

		HolePosition = LayoutData.DungeonTransform.TransformPosition(HolePosition);
		
		FDAVoxelShape HoleShape = FDAVoxelShape::CreateCeilingHole(HolePosition, CeilingHoleRadius, HoleMinHeight,
			EDAVoxelOperation::Subtract, EVoxelShapeLayer::PrimaryCarve);
		OutAdditionalShapes.Add(HoleShape);
	}
}

void UDungeonVoxelSDFModel_Cave::WriteShaderParameters(FVoxelDensityComputeShader::FParameters* Parameters) {
	Super::WriteShaderParameters(Parameters);

	Parameters->SDFModelType = 0;
}

// Island SDF Model Implementation
float UDungeonVoxelSDFModel_Island::GetBaseSDF(const FVector& WorldPos) const
{
	// Islands start with empty world (positive = empty in standard SDF)
	return 10000.0f;
}

float UDungeonVoxelSDFModel_Island::CombinePrimarySDF(float BaseSDF, float ShapeSDF) const
{
	// Islands add solid platforms to empty world
	// ShapeSDF is negative inside shapes (where we want solid)
	// Use Min to make those areas solid
	return FMath::Min(BaseSDF, ShapeSDF);
}

void UDungeonVoxelSDFModel_Island::ModifyPrimaryShape(FDAVoxelShape& Shape) const
{
	// For islands, we create raised platforms
	// The shape extends DOWN from its position to create a solid platform

	// Extend the platform deeper into the water
	// TODO make this parameterized
	constexpr float DesiredZ = -2000.0f;
	const float CurrentZ = Shape.Transform.GetLocation().Z;
	const float PlatformHeight = CurrentZ - DesiredZ;

	// Move the shape down so the top is at ground level
	Shape.Transform.SetLocation(Shape.Transform.GetLocation() - FVector(0, 0, PlatformHeight));

	// Set the height to extend deep enough
	Shape.Height = PlatformHeight;

	// Islands ADD solid geometry, not subtract/carve
	Shape.Operation = EDAVoxelOperation::Add;
}

FDungeonVoxelVerticalOffsets UDungeonVoxelSDFModel_Island::GetVerticalOffsets() const
{
	FDungeonVoxelVerticalOffsets Offsets;
	Offsets.TopOffset = 500.0f;  // Some height above for aerial view
	Offsets.BottomOffset = 2000.0f;  // Match the extended platform depth
	return Offsets;
}

float UDungeonVoxelSDFModel_Island::GetTotalCeilingHeight(float BaseRoomHeight) const
{
	// For islands: just ground level (0), no room concept
	// Volumes should be placed at ground level
	return 0.0f;
}

FVector UDungeonVoxelSDFModel_Island::GetChunkSearchBoundsExpansion(float NoiseAmplitude) const
{
	// Start with base noise expansion
	FVector Expansion = FVector(NoiseAmplitude, NoiseAmplitude, NoiseAmplitude);

	// Add horizontal expansion for shoreline extent
	// Shoreline can extend horizontally based on configured extent
	Expansion.X = FMath::Max(Expansion.X, ShorelineExtent);
	Expansion.Y = FMath::Max(Expansion.Y, ShorelineExtent);

	// Add vertical expansion for shoreline depth
	// Use the depth cutoff to limit how deep we search for chunks
	// This prevents creating unnecessary chunks deep underwater
	float ActualMaxDepth = FMath::Min(ShoreDepthCutoff, MaxShoreDepth);

	// Also consider platform depth but cap it with the cutoff
	float TotalDepth = FMath::Min(ActualMaxDepth + PlatformDepth, ShoreDepthCutoff + PlatformDepth);
	Expansion.Z = FMath::Max(Expansion.Z, TotalDepth);

	return Expansion;
}

void UDungeonVoxelSDFModel_Island::WriteShaderParameters(FVoxelDensityComputeShader::FParameters* Parameters) {
	Super::WriteShaderParameters(Parameters);
	Parameters->SDFModelType = 1;

	Parameters->WaterLevel = WaterLevel;
	Parameters->ShoreSlope = ShoreSlope;
	Parameters->ShorelineFalloffDistance = ShorelineFalloffDistance;
	Parameters->MaxShoreDepth = MaxShoreDepth;
	Parameters->ShoreDepthCutoff = ShoreDepthCutoff;
}

float UDungeonVoxelSDFModel_Island::ApplyWorldSpecificSDF(float CurrentSDF, const FVector& WorldPos) const
{
	if (CurrentSDF < 0) {
		return CurrentSDF;
	}

	if (WorldPos.Z >= WaterLevel) {
		//return FMath::Min(CurrentSDF, WorldPos.Z);
		return CurrentSDF;
	}
	
	float DistanceFromGeometry = CurrentSDF;
	float DepthBelowWater = WaterLevel - WorldPos.Z;
	
	float FalloffAlpha = 1.0f;
	if (ShorelineFalloffDistance > 0.0f && DistanceFromGeometry > ShorelineFalloffDistance) {
		// Beyond falloff distance, gradually reduce shore effect
		float ExcessDistance = DistanceFromGeometry - ShorelineFalloffDistance;
		float FalloffRange = ShorelineFalloffDistance * 0.5f;
		FalloffAlpha = FMath::Clamp(1.0f - (ExcessDistance / FalloffRange), 0.0f, 1.0f);

		// If completely beyond falloff, just return original SDF
		if (FalloffAlpha <= 0.0f) {
			return CurrentSDF;
		}
	}

	// Calculate shore depth using an easing curve for natural shoreline
	// Use a reasonable shore extent for normalization (not the falloff distance)
	float ShoreExtent = MaxShoreDepth / ShoreSlope; // How far horizontally the shore extends
	float NormalizedDistance = FMath::Clamp(DistanceFromGeometry / ShoreExtent, 0.0f, 1.0f);

	// Apply smoothstep (ease-in-out) curve - starts shallow, steep in middle, flattens at depth
	// This creates a natural beach profile: gentle slope near shore, steeper descent, then levels out
	float EasedDistance = NormalizedDistance * NormalizedDistance * (3.0f - 2.0f * NormalizedDistance);

	// Scale by max shore depth
	float ShoreDepth = EasedDistance * MaxShoreDepth;

	// Clamp shore depth to maximum allowed
	ShoreDepth = FMath::Min(ShoreDepth, MaxShoreDepth);

	// Shore surface height at this point (starts at water level, goes down)
	float ShoreHeight = WaterLevel - ShoreDepth;

	// Apply depth-based falloff to smoothly fade out shore effect at depth cutoff
	float DepthFalloff = 1.0f;
	if (DepthBelowWater > MaxShoreDepth) {
		// Start fading out after MaxShoreDepth
		float FadeRange = ShoreDepthCutoff - MaxShoreDepth;
		if (FadeRange > 0.0f) {
			DepthFalloff = FMath::Clamp(1.0f - (DepthBelowWater - MaxShoreDepth) / FadeRange, 0.0f, 1.0f);
		}
	}

	// Calculate SDF to the shore surface
	float ShoreSDF;
	if (WorldPos.Z <= ShoreHeight) {
		// Below shore surface - make it solid
		ShoreSDF = FMath::Min(CurrentSDF, WorldPos.Z - ShoreHeight);
	} else {
		// Above shore surface - use regular plane equation
		ShoreSDF = WorldPos.Z - ShoreHeight;
	}

	// Combine both falloffs
	float CombinedFalloff = FalloffAlpha * DepthFalloff;

	// Blend between original SDF and shore SDF based on combined falloff
	float BlendedSDF = FMath::Lerp(CurrentSDF, FMath::Min(CurrentSDF, ShoreSDF), CombinedFalloff);

	return BlendedSDF;
}


// Floating Island SDF Model Implementation

// Island SDF Model Implementation
float UDungeonVoxelSDFModel_FloatingIsland::GetBaseSDF(const FVector& WorldPos) const
{
	// Islands start with empty world (positive = empty in standard SDF)
	return 10000.0f;
}

float UDungeonVoxelSDFModel_FloatingIsland::CombinePrimarySDF(float BaseSDF, float ShapeSDF) const
{
	// Islands add solid platforms to empty world
	// ShapeSDF is negative inside shapes (where we want solid)
	// Use Min to make those areas solid
	return FMath::Min(BaseSDF, ShapeSDF);
}

void UDungeonVoxelSDFModel_FloatingIsland::ModifyPrimaryShape(FDAVoxelShape& Shape) const
{
	// For islands, we create raised platforms
	// The shape extends DOWN from its position to create a solid platform

	// Extend the platform deeper into the water
	const float DesiredZ = -PlatformDepth;
	const float CurrentZ = Shape.Transform.GetLocation().Z;
	const float PlatformHeight = CurrentZ - DesiredZ;

	// Move the shape down so the top is at ground level
	Shape.Transform.SetLocation(Shape.Transform.GetLocation() - FVector(0, 0, PlatformHeight));

	// Set the height to extend deep enough
	Shape.Height = PlatformHeight;

	// Islands ADD solid geometry, not subtract/carve
	Shape.Operation = EDAVoxelOperation::Add;
}

FDungeonVoxelVerticalOffsets UDungeonVoxelSDFModel_FloatingIsland::GetVerticalOffsets() const
{
	FDungeonVoxelVerticalOffsets Offsets;
	Offsets.TopOffset = 0.0f;  // Some height above for aerial view
	Offsets.BottomOffset = PlatformDepth;  // Match the extended platform depth
	return Offsets;
}

float UDungeonVoxelSDFModel_FloatingIsland::GetTotalCeilingHeight(float BaseRoomHeight) const
{
	// For islands: just ground level (0), no room concept
	// Volumes should be placed at ground level
	return 0.0f;
}

FVector UDungeonVoxelSDFModel_FloatingIsland::GetChunkSearchBoundsExpansion(float NoiseAmplitude) const
{
	const float Expansion = ShapeSearchExpansionAmount + NoiseAmplitude;
	return FVector(Expansion);
}

FVector UDungeonVoxelSDFModel_FloatingIsland::WarpBaseWorldPosition(const FVector& InWorldPos) {
	if (InWorldPos.Z >= SurfaceZ) {
		return InWorldPos;
	}
	
	FVector WorldPos = InWorldPos;
	FVector TipLocation = GetTipLocation();

	float SurfaceToTipDistance = SurfaceZ - TipLocation.Z;
	float SmoothStepTipToSurface = FMath::SmoothStep<float>(TipLocation.Z, SurfaceZ, WorldPos.Z);
	float SmoothStepSurfaceToTip = 1 - SmoothStepTipToSurface;
	float ScaleXY = 1;
	FQuat Rotation = FQuat::Identity;
	if (WorldPos.Z < SurfaceZ) {
		float Exp = ExpStrength * SmoothStepSurfaceToTip;
		ScaleXY = FMath::Exp2(Exp);

		//float DepthFromSurface = SurfaceZ - WorldPos.Z;
		float DepthFromSurface = SmoothStepSurfaceToTip * SurfaceToTipDistance;
		float DepthFromSurfaceMeters = DepthFromSurface / 100.0f;
		float AngleZ = DepthFromSurfaceMeters * CurlAnglePerMeter;

		Rotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(AngleZ));
	}
	
	FVector TipLocation2D(TipLocation.X, TipLocation.Y, 0);
	
	WorldPos -= TipLocation2D;
	WorldPos *= FVector(ScaleXY, ScaleXY, 1);
	WorldPos = Rotation.RotateVector(WorldPos);
	WorldPos += TipLocation2D;

	return WorldPos;
}

void UDungeonVoxelSDFModel_FloatingIsland::WriteShaderParameters(FVoxelDensityComputeShader::FParameters* Parameters) {
	Super::WriteShaderParameters(Parameters);
	Parameters->SDFModelType = 2;

	Parameters->FloatingIsland_PlatformDepth = PlatformDepth;
	Parameters->FloatingIsland_SurfaceZ = SurfaceZ;
	Parameters->FloatingIsland_ExpStrength = ExpStrength;
	Parameters->FloatingIsland_TipLocation = FVector3f(GetTipLocation());
	Parameters->FloatingIsland_CurlAnglePerMeterRad = FMath::DegreesToRadians(CurlAnglePerMeter);
}

void UDungeonVoxelSDFModel_FloatingIsland::Initialize(ADungeon* InDungeon) {
	Super::Initialize(InDungeon);

	if (InDungeon) {
		if (UDungeonModel* DungeonModel = InDungeon->GetModel()) {
			FVector LayoutCenter = DungeonModel->DungeonLayout.Bounds.GetCenter();
			DefaultTipLocation.X = LayoutCenter.X;
			DefaultTipLocation.Y = LayoutCenter.Y;
			DefaultTipLocation.Z = InDungeon->GetActorLocation().Z - PlatformDepth;
		}
	}
}

FVector UDungeonVoxelSDFModel_FloatingIsland::GetTipLocation() const {
	return TipLocationActor ? TipLocationActor->GetActorLocation() : DefaultTipLocation;
}

float UDungeonVoxelSDFModel_FloatingIsland::ApplyWorldSpecificSDF(float CurrentSDF, const FVector& WorldPos) const
{
	return CurrentSDF;
}
