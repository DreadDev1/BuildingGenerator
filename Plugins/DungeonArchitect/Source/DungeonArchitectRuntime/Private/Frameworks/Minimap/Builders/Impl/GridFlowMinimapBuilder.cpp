//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Minimap/Builders/Impl/GridFlowMinimapBuilder.h"

#include "Builders/GridFlow/GridFlowModel.h"
#include "Frameworks/Flow/Domains/Tilemap/FlowTilemapRenderer.h"
#include "Frameworks/FlowImpl/GridFlow/Tilemap/GridFlowTilemap.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "TextureResource.h"

void FGridFlowMinimapBuilder::BuildLayoutTexture(UWorld* InWorld, const FTransform& WorldToScreen, const FLayoutBuildSettings& InSettings,
		const FDungeonLayoutData& LayoutData, const UDungeonModel* InDungeonModel, TObjectPtr<UTexture>& OutMaskTexture)
{
	const UGridFlowModel* GridFlowModel = Cast<UGridFlowModel>(InDungeonModel);
	if (!GridFlowModel) {
		return;
	}
    
	UTextureRenderTarget2D* MaskTextureRTT = UKismetRenderingLibrary::CreateRenderTarget2D(InWorld, InSettings.TextureSize, InSettings.TextureSize, RTF_RGBA8,
																   FLinearColor::Transparent, false);

	FFlowTilemapRendererSettings Settings;
	Settings.bUseTextureTileSize = false;
	Settings.TileSize = 10;
	Settings.BackgroundColor = FLinearColor::Transparent;

	UTextureRenderTarget2D* RenderedRTT = FFlowTilemapRenderer::Create(GridFlowModel->Tilemap, Settings);
	RenderedRTT->SRGB = 0;
	RenderedRTT->Filter = TF_Nearest;

	FDrawToRenderTargetContext RenderContext;
	FVector2D CanvasSize;
	UCanvas* Canvas = nullptr;

	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(InWorld, MaskTextureRTT, Canvas, CanvasSize, RenderContext);
	float Aspect = RenderedRTT->GetSurfaceWidth() / RenderedRTT->GetSurfaceHeight();
	float Width = CanvasSize.X;
	int32 Height = CanvasSize.Y;
	if (Aspect > 1.0f) {
		Height /= Aspect;
	}
	else {
		Width *= Aspect;
	}

	FCanvasTileItem Item(FVector2D::ZeroVector, RenderedRTT->GetResource(), FVector2D(Width, Height), FLinearColor::White);
	Canvas->DrawItem(Item);
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(InWorld, RenderContext);
	UKismetRenderingLibrary::ReleaseRenderTarget2D(RenderedRTT);

	if (UTextureRenderTarget2D* OldMaskTextureRTT = Cast<UTextureRenderTarget2D>(OutMaskTexture)) {
		UKismetRenderingLibrary::ReleaseRenderTarget2D(OldMaskTextureRTT);
	}

	OutMaskTexture = MaskTextureRTT;
	OutMaskTexture->SRGB = 0;
	OutMaskTexture->Filter = TF_Nearest;
}

