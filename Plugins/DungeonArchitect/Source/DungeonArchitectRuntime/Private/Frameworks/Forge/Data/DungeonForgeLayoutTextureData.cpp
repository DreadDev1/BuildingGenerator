//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Data/DungeonForgeLayoutTextureData.h"

#include "Core/Utils/Debug/DungeonDebugVisualizer.h"
#include "Frameworks/Forge/Data/DungeonForgeLayoutTextureData.h"

#include "Engine/CanvasRenderTarget2D.h"
#include "Materials/MaterialInterface.h"

#if WITH_EDITOR
void UDungeonForgeLayoutTextureData::WriteDebugData(UDungeonDebugVisualizer* InDebugVisualizer) const {
	InDebugVisualizer->DrawBox(LocalBounds, WorldTransform.ToMatrixNoScale(), FLinearColor::Red, SDPG_Foreground);
	
	InDebugVisualizer->DrawTexturedQuad(GetTextureWorldTransform(), FloorTextures.SDF, DebugMaterial.LoadSynchronous());
}

#endif // WITH_EDITOR


FTransform UDungeonForgeLayoutTextureData::GetTextureWorldTransform() const {
	FVector BoundsLowerCenter = LocalBounds.GetCenter();
	BoundsLowerCenter.Z = LocalBounds.Min.Z;
	FVector Location = WorldTransform.TransformPosition(BoundsLowerCenter);
	
	FVector Scale = LocalBounds.GetExtent() * 2;
	const float ScaleXY = FMath::Max(Scale.X, Scale.Y);
	Scale.X = Scale.Y = ScaleXY;
	
	return FTransform(
		WorldTransform.GetRotation(),
		Location,
		Scale);
}

