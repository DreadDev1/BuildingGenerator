//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Markers/DungeonMarkerVisualization.h"

#include "SceneManagement.h"

FVector2D FDungeonMarkerVisualizer::LocalSquarePoints[4] = {
	FVector2D(-1.0f, -1.0f),  // Bottom Left
	FVector2D(1.0f, -1.0f),   // Bottom Right
	FVector2D(1.0f, 1.0f),    // Top Right
	FVector2D(-1.0f, 1.0f)    // Top Left
};

FDungeonMarkerVisualizationData FDungeonMarkerVisualizationRules::Default = { EDungeonMarkerVisualizationShape::Point };
FDungeonMarkerVisualizationData FDungeonMarkerVisualizationRules::Get(const FString& MarkerName) const {
	if (const FDungeonMarkerVisualizationData* FoundData = MarkerVisualizations.Find(MarkerName)) {
		return *FoundData;
	}
	return Default;
}

void FDungeonMarkerVisualizationRules::Set(const FString& InMarkerName, const FDungeonMarkerVisualizationData& InData) {
	MarkerVisualizations.FindOrAdd(InMarkerName) = InData;
}

FDungeonMarkerVisualizer::FDungeonMarkerVisualizer()
{
	
}

void FDungeonMarkerVisualizer::RenderMarker(FPrimitiveDrawInterface* PDI, const FString& InMarkerName, const FTransform& InMarkerTransform, const FDungeonMarkerVisualizationRules& InRules) {
	PDI->SetHitProxy(nullptr);
	const FDungeonMarkerVisualizationData Rule = InRules.Get(InMarkerName);
	const FVector GridSize = InRules.GetGridSize();
	if (Rule.VisualizationShape == EDungeonMarkerVisualizationShape::Tile) {
		RenderTile(PDI, InMarkerTransform, GridSize);
	}
	else if (Rule.VisualizationShape == EDungeonMarkerVisualizationShape::Wall) {
		RenderWall(PDI, InMarkerTransform, GridSize, Rule.Offset);
	}
	else if (Rule.VisualizationShape == EDungeonMarkerVisualizationShape::Pillar) {
		RenderPillar(PDI, InMarkerTransform, GridSize, Rule.Offset);
	}
	else if (Rule.VisualizationShape == EDungeonMarkerVisualizationShape::Point) {
		RenderPoint(PDI, InMarkerTransform, true);
	}
}

void FDungeonMarkerVisualizer::SetDepthPriorityGroup(const ESceneDepthPriorityGroup InDepthPriorityGroup) {
	this->DepthPriorityGroup = InDepthPriorityGroup;
}

void FDungeonMarkerVisualizer::SetDrawColor(const FLinearColor& InDrawColor) {
	this->DrawColor = InDrawColor;
}

void FDungeonMarkerVisualizer::SetLineThickness(const float InLineThickness) {
	this->LineThickness = InLineThickness;
}

void FDungeonMarkerVisualizer::SetPointThickness(const float InPointThickness) {
	this->PointThickness = InPointThickness;
}

void FDungeonMarkerVisualizer::SetDepthBias(float InDepthBias) {
	DepthBias = InDepthBias;
}

void FDungeonMarkerVisualizer::SetDrawScreenSpace(bool bInScreenSpace) {
	bDrawScreenSpace = bInScreenSpace;
}

void FDungeonMarkerVisualizer::RenderTile(FPrimitiveDrawInterface* PDI, const FTransform& InMarkerTransform, const FVector& InGridSize) {
	PDI->SetHitProxy(nullptr);
	RenderPoint(PDI, InMarkerTransform, false);

	auto LocalToWorld = [&](const FVector2D& InLocalPoint2D) {
		FVector LocalDir(InLocalPoint2D.X, InLocalPoint2D.Y, 0);
		FVector WorldDir = InMarkerTransform.TransformVector(LocalDir);
		FVector Location = InMarkerTransform.GetLocation();
		Location += WorldDir * 0.5f * InGridSize;
		return Location;
	};

	for (int i = 0; i < 4; i++) {
		FVector P0 = LocalToWorld(LocalSquarePoints[i]);
		FVector P1 = LocalToWorld(LocalSquarePoints[(i + 1) % 4]);
		PDI->DrawLine(P0, P1, DrawColor, DepthPriorityGroup, LineThickness, DepthBias, bDrawScreenSpace);
	}
}

void FDungeonMarkerVisualizer::RenderWall(FPrimitiveDrawInterface* PDI, const FTransform& InMarkerTransform, const FVector& InGridSize, const FTransform& InOffset) {
	auto LocalToWorld = [&](const FVector2D& InLocalPoint2D) {
		FVector LocalOffset(InLocalPoint2D.X, 0, InLocalPoint2D.Y);
		LocalOffset = InOffset.TransformPosition(LocalOffset);
		FVector WorldOffset = InMarkerTransform.TransformVector(LocalOffset);
		FVector Location = InMarkerTransform.GetLocation();
		Location += WorldOffset * InGridSize;

		return Location;
	};

	RenderPoint(PDI, InMarkerTransform, false);

	static const FVector2D LocalWallPoints[4] = {
		FVector2D(-0.5f, 0.0f),
		FVector2D(0.5f, 0.0f),
		FVector2D(0.5f, 2.0f),
		FVector2D(-0.5f, 2.0f) 
	};

	for (int i = 0; i < 4; i++) {
		FVector P0 = LocalToWorld(LocalWallPoints[i]);
		FVector P1 = LocalToWorld(LocalWallPoints[(i + 1) % 4]);
		PDI->DrawLine(P0, P1, DrawColor, DepthPriorityGroup, LineThickness, DepthBias, bDrawScreenSpace);
	}
}

void FDungeonMarkerVisualizer::RenderPillar(FPrimitiveDrawInterface* PDI, const FTransform& InMarkerTransform, const FVector& InGridSize, const FTransform& InOffset) {
	FVector P0 = InMarkerTransform.GetLocation() + InOffset.GetLocation() * InGridSize;
	FVector P1 = P0 + FVector(0, 0, InGridSize.Z * 2 * InOffset.GetScale3D().Z);
	RenderPoint(PDI, P0, true);
	PDI->DrawLine(P0, P1, DrawColor, DepthPriorityGroup, LineThickness, DepthBias, bDrawScreenSpace);
}

void FDungeonMarkerVisualizer::RenderPoint(FPrimitiveDrawInterface* PDI, const FTransform& InMarkerTransform, bool bDrawCircle) {
	RenderPoint(PDI, InMarkerTransform.GetLocation(), bDrawCircle);
}

void FDungeonMarkerVisualizer::RenderPoint(FPrimitiveDrawInterface* PDI, const FVector& InLocation, bool bDrawCircle) {
	if (!PDI) {
		return;
	}

	PDI->DrawPoint(InLocation, DrawColor, PointThickness, DepthPriorityGroup);

	if (bDrawCircle) {
		constexpr float CircleRadius = 20.0f;
		constexpr int32 CircleSegments = 16;
		constexpr float AnglePerSegment = 1.0f / static_cast<float>(CircleSegments) * PI * 2;
		auto GetPointAtSegment = [&](int32 SegmentIndex) {
			const float Angle = AnglePerSegment * SegmentIndex;
			FVector Delta = FVector(
				FMath::Sin(Angle) * CircleRadius,
				FMath::Cos(Angle) * CircleRadius,
				0);
			return InLocation + Delta; 
		};
		auto DrawLine = [&](int32 Segment0, int32 Segment1) {
			FVector P0 = GetPointAtSegment(Segment0);
			FVector P1 = GetPointAtSegment(Segment1);
			PDI->DrawLine(P0, P1, DrawColor, DepthPriorityGroup, PointThickness / 2, DepthBias, bDrawScreenSpace);
		};
		
		for (int32 i = 0; i < CircleSegments; i++) {
			DrawLine(i, (i + 1) % CircleSegments);
		}

		int32 HalfNumSegments = CircleSegments / 2;
		int32 QuarterNumSegments = HalfNumSegments / 2;
		DrawLine(0, HalfNumSegments);
		DrawLine(QuarterNumSegments, QuarterNumSegments + HalfNumSegments);
	}
}

