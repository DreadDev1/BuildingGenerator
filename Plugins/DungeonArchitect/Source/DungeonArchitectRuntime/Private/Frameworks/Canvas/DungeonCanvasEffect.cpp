//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/DungeonCanvasEffect.h"

#include "Engine/CanvasRenderTarget2D.h"

//////////////////////////////// DungeonCanvasEffectBase ////////////////////////////////
void UDungeonCanvasEffectBase::Initialize_Implementation() {
}

void UDungeonCanvasEffectBase::Draw_Implementation() {
}

void UDungeonCanvasEffectBase::Tick_Implementation(float DeltaSeconds) {
}

void UDungeonCanvasEffectBase::InitCanvasMaterial_Implementation(UMaterialInstanceDynamic* Material) {
}

void UDungeonCanvasEffectBase::SetTextures(UCanvasRenderTarget2D* TexLayoutFill, UCanvasRenderTarget2D* TexLayoutBorder, UCanvasRenderTarget2D* TexSDF, UCanvasRenderTarget2D* TexDynamicOcclusion) {
	LayoutFillTexture = TexLayoutFill;
	LayoutBorderTexture = TexLayoutBorder;
	SDFTexture = TexSDF;
	DynamicOcclusionTexture = TexDynamicOcclusion;
}

