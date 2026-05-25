//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Data/DungeonForgeTextureData.h"

#include "Core/Data/DungeonSpatialMapData.h"
#include "Core/Utils/Debug/DungeonDebugVisualizer.h"

#include "Materials/MaterialInterface.h"
#include "TextureResource.h"

#if WITH_EDITOR
void UDungeonForgeTextureData::WriteDebugData(UDungeonDebugVisualizer* InDebugVisualizer) const {
	if (DebugPreviewTexture.IsValid()) {
		InDebugVisualizer->DrawTexturedQuad(GetTextureWorldTransform(), DebugPreviewTexture.Get(), DebugMaterial.LoadSynchronous());
	}

	constexpr float PointSize = 10;
	if (Points.Num() > 0) {
		for (const FDungeonForgePoint& Point : Points) {
			FLinearColor Color = FMath::Lerp(FLinearColor::Red, FLinearColor::Blue, Point.Density);
			InDebugVisualizer->BatchedPoints.Emplace(Point.Transform.GetLocation(), Color, PointSize, 0, SDPG_Foreground);
			InDebugVisualizer->MarkRenderStateDirty();
		}
	}
}
#endif // WITH_EDITOR

FTransform UDungeonForgeTextureData::GetTextureWorldTransform() const {
	FTransform Transform = WorldTransform;
	Transform.SetScale3D(Transform.GetScale3D() * BaseScale);
	return Transform;
}

