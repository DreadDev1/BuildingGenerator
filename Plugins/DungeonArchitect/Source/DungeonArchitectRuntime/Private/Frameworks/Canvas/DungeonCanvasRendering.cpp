//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/DungeonCanvasRendering.h"

#include "Core/Dungeon.h"
#include "Core/DungeonBuilder.h"
#include "Core/Layout/DungeonDoorInterface.h"
#include "Core/Layout/DungeonLayoutData.h"
#include "Frameworks/Canvas/DungeonCanvas.h"
#include "Frameworks/Canvas/DungeonCanvasStructs.h"
#include "Frameworks/Canvas/DungeonCanvasTrackedObject.h"
#include "Frameworks/Canvas/DungeonCanvasViewport.h"
#include "Frameworks/Canvas/Shaders/CanvasFogOfWarShader.h"
#include "Frameworks/Canvas/Shaders/CanvasSDFEffectShaders.h"
#include "Frameworks/Canvas/Shaders/CanvasShapeBorderGenShader.h"
#include "Frameworks/Canvas/Shaders/JFAShader.h"
#include "Frameworks/Canvas/Themes/DungeonCanvasMaterialTheme.h"
#include "Frameworks/Canvas/UI/DungeonCanvasActorIconFilter.h"
#include "Frameworks/Lib/Shaders/BlitShader.h"
#include "Frameworks/Lib/Shaders/ConvoluteShader.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GlobalRenderResources.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"
#include "TextureResource.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogDungeonCanvasRendering, Log, All)


/////////////////////////// UDungeonCanvasRenderingLibrary ///////////////////////////
namespace DungeonCanvasRenderingLib {
	void CreateTextureIfNeeded(UObject* OuterOwner, int32 TextureWidth, int32 TextureHeight, TObjectPtr<UCanvasRenderTarget2D>& Texture, ETextureRenderTargetFormat TextureFormat) {
		if (Texture && Texture->GetSurfaceWidth() == TextureWidth && Texture->GetSurfaceHeight() == TextureHeight && Texture->RenderTargetFormat == TextureFormat) {
			// We can reuse this texture
			return;
		}

		// Destroy the existing texture
		if (Texture) {
			Texture->ReleaseResource();
		}

		const FLinearColor ClearColor = FLinearColor::Black;
		Texture = NewObject<UCanvasRenderTarget2D>(OuterOwner);
		Texture->RenderTargetFormat = TextureFormat;
		Texture->InitAutoFormat(TextureWidth, TextureHeight);
		Texture->ClearColor = ClearColor;
		Texture->UpdateResourceImmediate(true);

		UE_LOG(LogDungeonCanvasRendering, Log, TEXT("Created Canvas Texture: %s"), *Texture->GetName());
	}

	void ReleaseTexture(TObjectPtr<UCanvasRenderTarget2D>& TextureRef) {
		if (TextureRef) {
			TextureRef->ReleaseResource();
			TextureRef = nullptr;
			UE_LOG(LogDungeonCanvasRendering, Log, TEXT("Releasing Canvas Render Texture"));
		}
	}

	typedef TFunction<void(UCanvas*, const FVector2D&)> TCanvasRenderCallback; 
	static void RenderCanvasTexture(UWorld* InWorld, UTextureRenderTarget2D* OutTexture, TCanvasRenderCallback Callback, bool bClear = false, const FLinearColor& InClearColor = FLinearColor::Black) {
		if (bClear) {
			UKismetRenderingLibrary::ClearRenderTarget2D(InWorld, OutTexture, FLinearColor::Black);
		}
		
		FVector2D CanvasSize;
		UCanvas* Canvas = nullptr;
		FDrawToRenderTargetContext RenderContext;
		UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(InWorld, OutTexture, Canvas, CanvasSize, RenderContext);

		Callback(Canvas, CanvasSize);
		
		UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(InWorld, RenderContext);
		OutTexture->Modify();
	}
	
	static void DrawShapesFill(UWorld* InWorld, const FDungeonLayoutDataChunkInfo& LayoutShapes, const FDungeonCanvasViewportTransform& ViewportTransform,
			UTextureRenderTarget2D* OutTexture, bool bClearRTT = true, const FLinearColor& InClearColor = FLinearColor::Black, const FTexture* ShapeTexture = GWhiteTexture, ESimpleElementBlendMode BlendMode = SE_BLEND_Additive,
			TFunction<FLinearColor(float)> FnGetPolyColor = [](float Z){ return FLinearColor::White; })
	{
		RenderCanvasTexture(InWorld, OutTexture, [&LayoutShapes, &ViewportTransform, ShapeTexture, BlendMode, FnGetPolyColor](UCanvas* Canvas, const FVector2D& CanvasSize) {
			// Calculate the bounds and create the texture transformer (to move world space coords to texture space coords)

			const FTransform WorldToCanvasTransform = ViewportTransform.GetWorldToCanvas();
			auto WorldToCanvas = [&WorldToCanvasTransform](const FVector& InVector)-> FVector2D {
				return FVector2D(WorldToCanvasTransform.TransformPosition(InVector));
			};
			
			//  Draw the layout shapes in the fill texture mask
			for (const FDAShapePolygon& ConvexPoly : LayoutShapes.ConvexPolys) {
				FLinearColor Color;
				{
					//FTransform ColorTransform = FTransform(FVector(0, 0, ConvexPoly.Height)) * ConvexPoly.Transform;
					Color = FnGetPolyColor(ConvexPoly.Transform.GetLocation().Z);
				}
				for (int i = 2; i < ConvexPoly.Points.Num(); i++) {
					const int i0 = 0;
					const int i1 = i - 1;
					const int i2 = i;
					FVector2D P0 = WorldToCanvas(ConvexPoly.Transform.TransformPosition(FVector(ConvexPoly.Points[i0], 0)));
					FVector2D P1 = WorldToCanvas(ConvexPoly.Transform.TransformPosition(FVector(ConvexPoly.Points[i1], 0)));
					FVector2D P2 = WorldToCanvas(ConvexPoly.Transform.TransformPosition(FVector(ConvexPoly.Points[i2], 0)));

					FCanvasTriangleItem Triangle(P0, P1, P2, ShapeTexture);
					Triangle.SetColor(Color);
					Canvas->DrawItem(Triangle);
				}
			}
			
			for (const FDAShapeCircle& Circle : LayoutShapes.Circles) {
				FVector2D P0 = WorldToCanvas(Circle.Transform.GetLocation());
				const float Radius = Circle.Radius; // * Circle.Transform.GetScale3D().X;
				FLinearColor Color;
				{
					//FTransform ColorTransform = FTransform(FVector(0, 0, Circle.Height)) * Circle.Transform;
					Color = FnGetPolyColor(Circle.Transform.GetLocation().Z);
				}
				constexpr int32 NumSegments = 32;
				for (int i = 1; i <= NumSegments; i++) {
					float AngleRad0 = (PI * 2) / NumSegments * i;
					float AngleRad1 = (PI * 2) / NumSegments * (i - 1);
					FVector2D P1 = WorldToCanvas(Circle.Transform.TransformPosition(Radius * FVector(FMath::Cos(AngleRad0), FMath::Sin(AngleRad0), 0)));
					FVector2D P2 = WorldToCanvas(Circle.Transform.TransformPosition(Radius * FVector(FMath::Cos(AngleRad1), FMath::Sin(AngleRad1), 0)));
					
					FCanvasTriangleItem Triangle(P0, P1, P2, ShapeTexture);
					Triangle.SetColor(Color);
					Canvas->DrawItem(Triangle);
				}
			}

			for (const FDAShapeTexture& TextureShape : LayoutShapes.CanvasShapeTextures) {
				UTexture2D* LoadedTextureMask = TextureShape.TextureMask.LoadSynchronous();
				FTextureResource* TexResource = LoadedTextureMask ? LoadedTextureMask->GetResource() : nullptr;

				if (!TexResource) {
					continue;
				}

				TArray<FVector2D> WorldLocations;
				WorldLocations.SetNum(4);
				for (int i = 0; i < 4; i++) {
					WorldLocations[i] = WorldToCanvas(TextureShape.Transform.TransformPosition(FDAShapeTexture::LocalQuadPoints[i]));
				}
				
				FLinearColor Color = FnGetPolyColor(TextureShape.Transform.GetLocation().Z);
				for (int i = 2; i < 4; i++) {
					const FVector2D& P0 = WorldLocations[0];
					const FVector2D& P1 = WorldLocations[i - 1];
					const FVector2D& P2 = WorldLocations[i];

					const FVector2D& T0 = FDAShapeTexture::QuadUV[0];
					const FVector2D& T1 = FDAShapeTexture::QuadUV[i - 1];
					const FVector2D& T2 = FDAShapeTexture::QuadUV[i];

					FCanvasTriangleItem Triangle(P0, P1, P2, T0, T1, T2, TexResource);
					Triangle.SetColor(Color);
					Triangle.BlendMode = BlendMode;
					Canvas->DrawItem(Triangle);
				}
			}
			
		}, bClearRTT, InClearColor);
	}

	
	void DrawCanvasIcon(UCanvas* Canvas, FDungeonCanvasLayoutRenderResources& RenderResources, const TFunction<void(UMaterialInstanceDynamic* MID)>& FuncSetupMaterial,
				EDungeonCanvasIconFogOfWarRenderMode FogRenderMode, UObject* ResourceObject, const FVector2D& InLocation, float ScreenSize, float AspectRatio,
				FLinearColor InColor, float Rotation)
	{
		if (!Canvas || !ResourceObject) return;
		if (FMath::IsNearlyZero(AspectRatio)) {
			AspectRatio = 1.0f;
		}
		
		float Width{};
		float Height{};
		
		if (AspectRatio >= 1.0f) {
			Width = ScreenSize;
			Height = ScreenSize / AspectRatio;
		}
		else {
			Width = ScreenSize * AspectRatio;
			Height = ScreenSize;
		}
		
		const FVector2D Size = FVector2D(Width, Height);
		const FVector2D CanvasLocation = InLocation - Size * 0.5f;

		auto LocalSetupMaterial = [&](UMaterialInstanceDynamic* MID) {
			FVector2D IconUVLoc = CanvasLocation / FVector2D(Canvas->SizeX, Canvas->SizeY);
			FVector2D IconUVSize = Size / FVector2D(Canvas->SizeX, Canvas->SizeY);
			
			MID->SetVectorParameterValue("IconUVLoc", FVector(IconUVLoc, 0));
			MID->SetVectorParameterValue("IconUVSize", FVector(IconUVSize, 0));
			MID->SetScalarParameterValue("FogRenderMode", static_cast<int32>(FogRenderMode));
		};
		
		if (UTexture* Texture = Cast<UTexture>(ResourceObject)) {
			{
				if (UMaterialInterface* TextureMaterialTemplate = RenderResources.TextureMaterialTemplate) {
					UMaterialInstanceDynamic* TextureIconMaterial = RenderResources.MaterialPool.LockMaterial(TextureMaterialTemplate);
					static const FName ParamTexture = TEXT("Texture");
					static const FName ParamTint = TEXT("Tint");
					TextureIconMaterial->SetTextureParameterValue(ParamTexture, Texture);
					TextureIconMaterial->SetVectorParameterValue(ParamTint, InColor);

					LocalSetupMaterial(TextureIconMaterial);
					
					// Set fog of war parameters
					if (FuncSetupMaterial) {
						FuncSetupMaterial(TextureIconMaterial);
					}
					
					Canvas->K2_DrawMaterial(TextureIconMaterial, CanvasLocation, Size, FVector2D::ZeroVector, FVector2D::UnitVector, Rotation);
				}
			}
			//Canvas->K2_DrawTexture(Texture, CanvasLocation, Size, FVector2D::ZeroVector, FVector2D::UnitVector, InColor, BlendMode, Rotation);
		}
		else if (UMaterialInterface* Material = Cast<UMaterialInterface>(ResourceObject)) {
			UMaterialInstanceDynamic* IconMaterialInstance = RenderResources.MaterialPool.LockMaterial(Material);

			LocalSetupMaterial(IconMaterialInstance);
			
			// Set fog of war parameters
			FuncSetupMaterial(IconMaterialInstance);
			
			Canvas->K2_DrawMaterial(IconMaterialInstance, CanvasLocation, Size, FVector2D::ZeroVector, FVector2D::UnitVector, Rotation);
		}
	}

	template<typename TShader>
	void TBlurTexture(UCanvasRenderTarget2D* SourceTexture, UCanvasRenderTarget2D* DestinationTexture) {
		if (!SourceTexture || !DestinationTexture
				|| SourceTexture->SizeX != DestinationTexture->SizeX
				|| SourceTexture->SizeY != DestinationTexture->SizeY) {
			return;
				}
				
		const FIntPoint TextureSize(SourceTexture->SizeX, SourceTexture->SizeY);
	
		const FIntVector ThreadGroupsFogBlur = FIntVector(
				FMath::DivideAndRoundUp(TextureSize.X, 16),
				FMath::DivideAndRoundUp(TextureSize.Y, 16),
				1);

	
		const FTextureResource* SourceTexResource = SourceTexture->GetResource();
		const FTextureResource* DestTexResource = DestinationTexture->GetResource();
	
		ENQUEUE_RENDER_COMMAND(DABlurTexture)(
			[=](FRHICommandListImmediate& RHICmdList) {
				FRDGBuilder GraphBuilder(RHICmdList);
			
				TRefCountPtr<IPooledRenderTarget> SrcTexCache, DstTexCache;
				CacheRenderTarget(SourceTexResource->TextureRHI, TEXT("Blur_SrcTex"), SrcTexCache);
				CacheRenderTarget(DestTexResource->TextureRHI, TEXT("Blur_DstTex"), DstTexCache);
			
				const FRDGTextureRef SrcTexRDG = GraphBuilder.RegisterExternalTexture(SrcTexCache);
				const FRDGTextureRef DstTexRDG = GraphBuilder.RegisterExternalTexture(DstTexCache);

				const FRDGTextureSRVRef SrcTexSRV = GraphBuilder.CreateSRV(SrcTexRDG);
				const FRDGTextureUAVRef DstTexUAV = GraphBuilder.CreateUAV(DstTexRDG);

				typename TShader::FParameters* FogBlurParams = GraphBuilder.AllocParameters<typename TShader::FParameters>();
				FogBlurParams->InTexture = SrcTexSRV;
				FogBlurParams->OutTexture = DstTexUAV;
				FogBlurParams->TextureWidth = TextureSize.X;
				FogBlurParams->TextureHeight = TextureSize.Y;
					
				const TShaderMapRef<TShader> FogBlurShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
				FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("CanvasBlur"), FogBlurShader, FogBlurParams, ThreadGroupsFogBlur);

				GraphBuilder.Execute();
			});
	}
};



UMaterialInstanceDynamic* FDungeonCanvasMaterialPool::LockMaterial(UMaterialInterface* InParent) {
	UMaterialInstanceDynamic* LockedMaterial = nullptr;
	for (UMaterialInstanceDynamic* FreeMaterial : FreeMaterials) {
		if (FreeMaterial && FreeMaterial->Parent == InParent) {
			LockedMaterial = FreeMaterial;
			break;
		}
	}

	if (!LockedMaterial) {
		LockedMaterial = UMaterialInstanceDynamic::Create(InParent, GetTransientPackage());
	}
	else {
		FreeMaterials.Remove(LockedMaterial);
	}

	LockedMaterials.Add(LockedMaterial);

	return LockedMaterial;
}

void FDungeonCanvasMaterialPool::UnlockAll() {
	FreeMaterials.Append(LockedMaterials);
	LockedMaterials.Reset();
}

void FDungeonCanvasMaterialPool::Clear() {
	FreeMaterials.Reset();
	LockedMaterials.Reset();
}

void FDungeonCanvasLayoutRenderResources::Reset() {
	FloorTextures.Reset();
	MaterialPool = {};
}

void FDungeonCanvasRenderingLibrary::CreateDungeonCanvasLayoutFloorTextures(UObject* OuterOwner, int32 TextureWidth, int32 TextureHeight, FDungeonCanvasLayoutFloorTextures& FloorTextures, const FFloorTexturesGenerationOptions& InOptions) {
	if (InOptions.bGenerateSDF) {
		DungeonCanvasRenderingLib::CreateTextureIfNeeded(OuterOwner, TextureWidth, TextureHeight, FloorTextures.SDF, RTF_R16f);
	}
	if (InOptions.bGenerateLayoutFill) {
		DungeonCanvasRenderingLib::CreateTextureIfNeeded(OuterOwner, TextureWidth, TextureHeight, FloorTextures.LayoutFill, RTF_R16f);
	}
	if (InOptions.bGenerateLayoutBorder) {
		DungeonCanvasRenderingLib::CreateTextureIfNeeded(OuterOwner, TextureWidth, TextureHeight, FloorTextures.LayoutBorder, RTF_R8);
	}
	if (InOptions.bGenerateDynamicOcclusion) {
		DungeonCanvasRenderingLib::CreateTextureIfNeeded(OuterOwner, TextureWidth, TextureHeight, FloorTextures.DynamicOcclusion, RTF_R8);
	}
	if (InOptions.bGenerateFogOfWarExplored) {
		DungeonCanvasRenderingLib::CreateTextureIfNeeded(OuterOwner, TextureWidth, TextureHeight, FloorTextures.FogOfWarExplored, RTF_R8);
	}
	if (InOptions.bGenerateFogOfWarVisibility) {
		DungeonCanvasRenderingLib::CreateTextureIfNeeded(OuterOwner, TextureWidth, TextureHeight, FloorTextures.FogOfWarVisibility, RTF_R8);
	}
}

void FDungeonCanvasRenderingLibrary::ReleaseDungeonCanvasLayoutResources(FDungeonCanvasLayoutRenderResources& LayoutResources) {
	for (auto& Entry : LayoutResources.FloorTextures) {
		FDungeonCanvasLayoutFloorTextures& Floor = Entry.Value;
		DungeonCanvasRenderingLib::ReleaseTexture(Floor.SDF);
		DungeonCanvasRenderingLib::ReleaseTexture(Floor.LayoutFill);
		DungeonCanvasRenderingLib::ReleaseTexture(Floor.LayoutBorder);
		DungeonCanvasRenderingLib::ReleaseTexture(Floor.DynamicOcclusion);
		DungeonCanvasRenderingLib::ReleaseTexture(Floor.FogOfWarExplored);
		DungeonCanvasRenderingLib::ReleaseTexture(Floor.FogOfWarVisibility);
	}
	LayoutResources.FloorTextures.Reset();
	LayoutResources.MaterialPool.Clear();
}

void FDungeonCanvasRenderingLibrary::DrawDungeonLayoutHeightTexture(UWorld* InWorld, const FDungeonLayoutData& FilteredDungeonLayout,
		const FDungeonCanvasViewportTransform& InFullDungeonTransform, float InHeightNormalizeBias, UTextureRenderTarget2D* OutLayoutFill,
		float& OutRenderedMinHeight, float& OutRenderedMaxHeight)
{
	if (!OutLayoutFill) {
		return;
	}

	// Accumulate all the shapes from the chunks and render them in a single batch
	FDungeonLayoutDataChunkInfo AllShapes;
	for (const FDungeonLayoutDataChunkInfo& ChunkShapeList : FilteredDungeonLayout.ChunkShapes) {
		AllShapes.Append(ChunkShapeList, { FDAShapeConstants::TagDoNotRenderOnCanvas });
	}

	// Draw the height in the layout texture, normalized from (0..1]    O here would imply empty, so the lowest floor will be greater than 0
	float MinHeight{};
	float MaxHeight{};
	{
		bool bRangeInitialized{};
		auto UpdateRange = [&](float Z, float ShapeHeight) {
			if (!bRangeInitialized) {
				MinHeight = Z;
				MaxHeight = Z + ShapeHeight;
				bRangeInitialized = true;
			}
			else {
				MinHeight = FMath::Min(MinHeight, Z);
				MaxHeight = FMath::Max(MaxHeight, Z + ShapeHeight);
			}
		};
		
		for (const FDungeonLayoutDataChunkInfo& ChunkShape : FilteredDungeonLayout.ChunkShapes) {
			for (const FDAShapePolygon& ConvexPoly : ChunkShape.ConvexPolys) {
				UpdateRange(ConvexPoly.Transform.GetLocation().Z, ConvexPoly.Height);
			}
			for (const FDAShapeCircle& Circle : ChunkShape.Circles) {
				UpdateRange(Circle.Transform.GetLocation().Z, Circle.Height);
			}
			for (const FDAShapeTexture& CanvasShapeTexture : ChunkShape.CanvasShapeTextures) {
				UpdateRange(CanvasShapeTexture.Transform.GetLocation().Z, 0);
			}
		}
	}

	OutRenderedMinHeight = MinHeight;
	OutRenderedMaxHeight = MaxHeight;
	
	auto FnGetPolyColor = [MinHeight, MaxHeight, InHeightNormalizeBias](float Z) {
		Z = FMath::Clamp(Z, MinHeight, MaxHeight);
		float Alpha{};
		if (!FMath::IsNearlyEqual(MinHeight, MaxHeight)) {
			Alpha = (Z - MinHeight) / (MaxHeight - MinHeight);
			Alpha = Alpha * (1.0f - InHeightNormalizeBias) + InHeightNormalizeBias;
		}
		else {
			Alpha = 1;
		}
		return FLinearColor(Alpha, Alpha, Alpha, 1);
	};
	
	DungeonCanvasRenderingLib::DrawShapesFill(InWorld, AllShapes, InFullDungeonTransform, OutLayoutFill, true, FLinearColor::Black, GWhiteTexture, SE_BLEND_Additive, FnGetPolyColor);
}

void FDungeonCanvasRenderingLibrary::DrawDungeonLayout(UWorld* InWorld, const FDungeonLayoutData& InDungeonLayout, float PaddingWorldUnits, bool bDrawAllFloors, int32 FloorIndex,
		const float InHeightNormalizeBias, UTextureRenderTarget2D* OutLayoutFill, UTextureRenderTarget2D* OutLayoutBorder, float& OutRenderedMinHeight, float& OutRenderedMaxHeight)
{
	if (!OutLayoutFill || !OutLayoutBorder) {
		return;
	}

	if (OutLayoutFill->GetSurfaceWidth() != OutLayoutBorder->GetSurfaceWidth() || OutLayoutFill->GetSurfaceHeight() != OutLayoutBorder->GetSurfaceHeight())
	{
		return;
	}
	
	// Setup the height range
	FDungeonLayoutHeightRange HeightRange;
	{
		FTransform DungeonTransform = InDungeonLayout.DungeonTransform;
		DungeonTransform.RemoveScaling();
		HeightRange.SetBaseHeightOffset(DungeonTransform.GetLocation().Z);
	
		if (bDrawAllFloors) {
			HeightRange.SetHeightRangeAcrossAllFloors();
		}
		else {
			HeightRange.SetHeightRangeForSingleFloor(InDungeonLayout.FloorSettings, FloorIndex);
		}
	}

	
	const FVector2D CanvasSize(OutLayoutFill->GetSurfaceWidth(), OutLayoutFill->GetSurfaceHeight());
	float LayoutTextureSize = FMath::Max(CanvasSize.X, CanvasSize.Y);
	
	FBox DungeonLayoutBounds = InDungeonLayout.Bounds.ExpandBy(FVector(PaddingWorldUnits, PaddingWorldUnits, 0));
	FDungeonCanvasViewportTransform FullDungeonTransform = {};
	FullDungeonTransform.FocusOnCanvas(LayoutTextureSize, LayoutTextureSize);
	FullDungeonTransform.SetLocalToWorld(FTransform(FRotator::ZeroRotator, DungeonLayoutBounds.GetCenter(), DungeonLayoutBounds.GetSize()));

	DrawDungeonLayout(InWorld, InDungeonLayout, FullDungeonTransform, HeightRange, InHeightNormalizeBias, OutLayoutFill, OutLayoutBorder, OutRenderedMinHeight, OutRenderedMaxHeight);
}

void FDungeonCanvasRenderingLibrary::DrawDungeonLayout(UWorld* InWorld, const FDungeonLayoutData& InDungeonLayout, const FDungeonCanvasViewportTransform& FullDungeonTransform,
		const FDungeonLayoutHeightRange& HeightRange, const float InHeightNormalizeBias, UTextureRenderTarget2D* OutLayoutFill, UTextureRenderTarget2D* OutLayoutBorder,
		float& OutRenderedMinHeight, float& OutRenderedMaxHeight)
{
	if (!OutLayoutFill || !OutLayoutBorder) {
		return;
	}

	if (OutLayoutFill->GetSurfaceWidth() != OutLayoutBorder->GetSurfaceWidth() || OutLayoutFill->GetSurfaceHeight() != OutLayoutBorder->GetSurfaceHeight())
	{
		return;
	}
	
	const FVector2D CanvasSize(OutLayoutFill->GetSurfaceWidth(), OutLayoutFill->GetSurfaceHeight());
	
	const FDungeonLayoutData& UnfilteredDungeonLayout = InDungeonLayout;
	const FDungeonLayoutData FilteredDungeonLayout = FDungeonLayoutUtils::FilterByHeightRange(UnfilteredDungeonLayout, HeightRange.GetMinHeight(), HeightRange.GetMaxHeight());
	
	DrawDungeonLayoutHeightTexture(InWorld, FilteredDungeonLayout, FullDungeonTransform, InHeightNormalizeBias, OutLayoutFill, OutRenderedMinHeight, OutRenderedMaxHeight);
	
	UKismetRenderingLibrary::ClearRenderTarget2D(InWorld, OutLayoutBorder, FLinearColor::Black);
	
	// Make a temporary fill texture
	UCanvasRenderTarget2D* TempFillTex = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(InWorld, UCanvasRenderTarget2D::StaticClass(),
			OutLayoutFill->GetSurfaceWidth(), OutLayoutFill->GetSurfaceHeight());
	
	FTextureResource* TempFillTexResource = TempFillTex->GetResource();
	FTextureResource* LayoutBorderResource = OutLayoutBorder->GetResource();
	
	// Draw the border for each chunk
	for (const FDungeonLayoutDataChunkInfo& ChunkShapeList : FilteredDungeonLayout.ChunkShapes) {
		FDungeonLayoutDataChunkInfo FilteredShapes;
		FilteredShapes.Append(ChunkShapeList, { FDAShapeConstants::TagDoNotRenderOnCanvas });
		// Fill the chunk shapes, then find the combined outline for that shape
		DungeonCanvasRenderingLib::DrawShapesFill(InWorld, FilteredShapes, FullDungeonTransform, TempFillTex, true);

		ENQUEUE_RENDER_COMMAND(WriteChunkOutlines)(
			[FilteredShapes, FullDungeonTransform, TempFillTexResource, LayoutBorderResource, CanvasSize](FRHICommandListImmediate& RHICmdList) {
				FRDGBuilder GraphBuilder(RHICmdList);
				
				auto WorldToCanvas = [&FullDungeonTransform](const FVector& InVector)-> FVector2D {
					const FTransform WorldToCanvasTransform = FullDungeonTransform.GetWorldToCanvas();
					return FVector2D(WorldToCanvasTransform.TransformPosition(InVector));
				};
				
				// Find the bounds of the chunk that we just wrote and run it through the edge detection shader
				FIntVector2 CanvasChunkMin, CanvasChunkSize;
				{
					FDungeonLayoutUtils::FCalcBoundsSettings CalcBoundsSettings;
					CalcBoundsSettings.bIncludeTextureOverlays = true;
					const FBox ChunkBounds = FDungeonLayoutUtils::CalculateBounds(FilteredShapes, CalcBoundsSettings);
		
					const FVector2D CanvasChunkMinF = WorldToCanvas(ChunkBounds.Min);
					const FVector2D CanvasChunkMaxF = WorldToCanvas(ChunkBounds.Max);

					CanvasChunkMin = FIntVector2(
						FMath::FloorToInt(CanvasChunkMinF.X),
						FMath::FloorToInt(CanvasChunkMinF.Y));

					const FIntVector2 CanvasChunkMax = FIntVector2(
						FMath::CeilToInt(CanvasChunkMaxF.X),
						FMath::CeilToInt(CanvasChunkMaxF.Y));
		
					CanvasChunkSize = CanvasChunkMax - CanvasChunkMin;
				}

				TRefCountPtr<IPooledRenderTarget> TempFillTexCache, LayoutBorderCache;
				CacheRenderTarget(TempFillTexResource->TextureRHI, TEXT("BorderGen_FillTex"), TempFillTexCache);
				CacheRenderTarget(LayoutBorderResource->TextureRHI, TEXT("BorderGen_BorderTex"), LayoutBorderCache);
				
				const FRDGTextureRef FillTexRDG = GraphBuilder.RegisterExternalTexture(TempFillTexCache);
				const FRDGTextureRef BorderTexRDG = GraphBuilder.RegisterExternalTexture(LayoutBorderCache);

				const FRDGTextureSRVRef FillTexSRV = GraphBuilder.CreateSRV(FillTexRDG);
				const FRDGTextureUAVRef BorderTexUAV = GraphBuilder.CreateUAV(BorderTexRDG);

				FDACanvasShapeBorderGenShader::FParameters* BorderGenParams = GraphBuilder.AllocParameters<FDACanvasShapeBorderGenShader::FParameters>();
				BorderGenParams->TextureWidth = CanvasSize.X;
				BorderGenParams->TextureHeight = CanvasSize.Y;

				BorderGenParams->OffsetX = CanvasChunkMin.X;
				BorderGenParams->OffsetY = CanvasChunkMin.Y;
				
				BorderGenParams->FillTexture = FillTexSRV;
				BorderGenParams->BorderTexture = BorderTexUAV;

				const int32 ThreadGroupsPerSideX = FMath::DivideAndRoundUp(CanvasChunkSize.X, 16);
				const int32 ThreadGroupsPerSideY = FMath::DivideAndRoundUp(CanvasChunkSize.Y, 16);
				const FIntVector ThreadGroups(ThreadGroupsPerSideX, ThreadGroupsPerSideY, 1);
				
				const TShaderMapRef<FDACanvasShapeBorderGenShader> BorderGenShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
				FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("BorderGen"), BorderGenShader, BorderGenParams, ThreadGroups);

				GraphBuilder.Execute();
			});
	}

	TempFillTex->ReleaseResource();
	TempFillTex = nullptr;

	/*
	 * For the doors:
	 * Draw black boxes on the outline texture (to remove the outlines where the doors occur)
	 * Draw white boxes on the fill texture, so disconnected rooms are connected by thick doors
	 * Finally, run the outline shader again on all the door bounds to connect the outlines of the doors, if the rooms were disconnected
	 */
	{
		// find out 1px = ? world units
		FVector PixelToWorldSize = FullDungeonTransform.GetCanvasToWorld().GetScale3D();

		TArray<FDungeonLayoutDataDoorItem> AllOpenings;
		AllOpenings.Append(FilteredDungeonLayout.Doors);
		AllOpenings.Append(FilteredDungeonLayout.WallOpening);
		
		FDungeonLayoutDataChunkInfo DoorShapes;
		for (const FDungeonLayoutDataDoorItem& DoorInfo : AllOpenings) {
			FVector2D DoorBoxSize(DoorInfo.Width, DoorInfo.DoorOcclusionThickness);
			//DoorBoxSize -= FVector2D(PixelToWorldSize) * 2;
			
			const TArray<FVector2D> LocalDoorPoints = {
				DoorBoxSize * FVector2D(-0.5, -0.5),
				DoorBoxSize * FVector2D( 0.5, -0.5),
				DoorBoxSize * FVector2D( 0.5,  0.5),
				DoorBoxSize * FVector2D(-0.5,  0.5),
			};
		
			FDAShapePolygon& DoorPoly = DoorShapes.ConvexPolys.AddDefaulted_GetRef();
			DoorPoly.Points = LocalDoorPoints;
			DoorPoly.Transform = FTransform(FRotator(0, 90, 0)) * DoorInfo.WorldTransform;
		}
		// Clear the outlines by drawing a black box around the doors
		DungeonCanvasRenderingLib::DrawShapesFill(InWorld, DoorShapes, FullDungeonTransform, OutLayoutBorder, false, FLinearColor::Black, GBlackTexture);
	}

	// Draw the stair openings
	{
		FDungeonLayoutDataChunkInfo StairShapes;
		for (const FDungeonLayoutDataStairItem& StairInfo : FilteredDungeonLayout.Stairs) {
			constexpr float StairSizeMultiplier = 0.9f;
			FVector2D StairBoxSize(StairInfo.Width * StairSizeMultiplier, StairInfo.Width * StairSizeMultiplier);
			const TArray<FVector2D> LocalDoorPoints = {
				StairBoxSize * FVector2D(-0.5, -0.5),
				StairBoxSize * FVector2D( 0.5, -0.5),
				StairBoxSize * FVector2D( 0.5,  0.5),
				StairBoxSize * FVector2D(-0.5,  0.5),
			};
		
			FDAShapePolygon& StairPoly = StairShapes.ConvexPolys.AddDefaulted_GetRef();
			StairPoly.Points = LocalDoorPoints;
			StairPoly.Transform = FTransform(FRotator(0, 90, 0), FVector(-StairInfo.Width * 0.5, 0, 0)) * StairInfo.WorldTransform;
		}
		// Clear the outlines by drawing a black box around the doors
		DungeonCanvasRenderingLib::DrawShapesFill(InWorld, StairShapes, FullDungeonTransform, OutLayoutBorder, false, FLinearColor::Black, GBlackTexture);
	}
}


void FDungeonCanvasRenderingLibrary::UpdateDynamicOcclusions(UWorld* InWorld, const TArray<FDungeonCanvasTrackedActorRegistryItem>& InTrackedItems, FDungeonCanvasLayoutRenderResources& RenderResources, const FDungeonLayoutHeightRange& HeightRange, UCanvasRenderTarget2D* DynamicOcclusionTexture,
                                                             const FDungeonCanvasViewportTransform& FullDungeonTransform, const TArray<FDungeonCanvasOverlayIcon>& OverlayIcons)
{
	if (!DynamicOcclusionTexture || !InWorld) {
		return;
	}

	FVector2D CanvasSize;
	UCanvas* Canvas = nullptr;
	FDrawToRenderTargetContext RenderContext;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(InWorld, DynamicOcclusionTexture, Canvas, CanvasSize, RenderContext);
	UKismetRenderingLibrary::ClearRenderTarget2D(InWorld, DynamicOcclusionTexture, FLinearColor::Black);
	
	//FDungeonCanvasViewportTransform View;
	//View.SetLocalToWorld(WorldBoundsTransform);
	//View.FocusOnCanvas(DynamicOcclusionTexture->SizeX, DynamicOcclusionTexture->SizeY);
	FDungeonCanvasViewportTransform View = FullDungeonTransform;
	
	TMap<FName, FDungeonCanvasOverlayIcon> IconMap;
	for (const FDungeonCanvasOverlayIcon& OverlayIcon : OverlayIcons) {
		FDungeonCanvasOverlayIcon& IconRef = IconMap.FindOrAdd(OverlayIcon.Name);
		IconRef = OverlayIcon;
	}
	
	for (const FDungeonCanvasTrackedActorRegistryItem& TrackedItem : InTrackedItems) {
		const UDungeonCanvasTrackedObject* TrackedObject = TrackedItem.TrackedComponent.Get();
		if (TrackedObject && TrackedObject->bOccludesFogOfWar && IconMap.Contains(TrackedObject->IconName)) {
			FVector WorldLocation = TrackedObject->GetComponentLocation();
			const FVector2D CanvasLocation = View.WorldToCanvasLocation(WorldLocation);

			if (const FDungeonCanvasOverlayIcon* SearchResult = IconMap.Find(TrackedObject->IconName)) {
				const FDungeonCanvasOverlayIcon& OverlayData = *SearchResult;
				if (!OverlayData.ResourceObject) {
					continue;
				}
				
				float Rotation = OverlayData.RotationOffset;
				if (TrackedObject->bOrientToRotation) {
					const FVector Angles = TrackedObject->GetComponentToWorld().GetRotation().Euler();
					Rotation += Angles.Z;
				}
				float ScreenSize = OverlayData.ScreenSize;
				if (OverlayData.ScreenSizeType == EDungeonCanvasIconCoordinateSystem::Pixels) {
					ScreenSize = OverlayData.ScreenSize;
				}
				else if (OverlayData.ScreenSizeType == EDungeonCanvasIconCoordinateSystem::WorldCoordinates) {
					const FVector CanvasScale3D = View.GetWorldToCanvas().GetScale3D() * OverlayData.ScreenSize;
					ScreenSize = FMath::Max(CanvasScale3D.X, CanvasScale3D.Y);
				}

				DungeonCanvasRenderingLib::DrawCanvasIcon(Canvas, RenderResources, {}, EDungeonCanvasIconFogOfWarRenderMode::Always,
						OverlayData.ResourceObject, CanvasLocation, ScreenSize, OverlayData.AspectRatio, OverlayData.Tint, Rotation);
			}
		}
	}

	// Draw the doors
	TSet<const AActor*> DoorActors;
	for (const FDungeonCanvasTrackedActorRegistryItem& TrackedItem : InTrackedItems) {
		if (const UDungeonCanvasTrackedObject* TrackedObject = TrackedItem.TrackedComponent.Get()) {
			if (const AActor* PossibleDoorActor = TrackedObject->GetOwner()) {
				if (PossibleDoorActor->GetClass()->ImplementsInterface(UDungeonDoorInterface::StaticClass())) {
					DoorActors.Add(PossibleDoorActor);
				}
			}
		}
	}

	for (const AActor* DoorActor : DoorActors) {
		if (HeightRange.GetRangeType() != FDungeonLayoutHeightRangeType::EntireDungeon) {
			const float ActorZ = DoorActor->GetActorLocation().Z;
			const float MinHeight = HeightRange.GetMinHeight();
			const float MaxHeight = HeightRange.GetMaxHeight();
			if (ActorZ < MinHeight || ActorZ > MaxHeight) {
				continue;
			}
		}
		
		bool bIsOpen = IDungeonDoorInterface::Execute_IsPassageOpen(DoorActor);
		if (!bIsOpen) {
			// Draw a line covering the door
			float DoorWidth = IDungeonDoorInterface::Execute_GetDoorWidth(DoorActor);
			float BaseRotation = IDungeonDoorInterface::Execute_GetDoorBaseRotation(DoorActor);
			const FTransform& ActorTransform = DoorActor->GetActorTransform();
			FTransform Transform =
					FTransform(FRotator(0, BaseRotation, 0))
					* ActorTransform;
			
			FVector LocalDoorPointA(0, -DoorWidth * 0.5f, 0);
			FVector LocalDoorPointB(0, DoorWidth * 0.5f, 0);

			FVector WorldDoorPointA = Transform.TransformPosition(LocalDoorPointA);
			FVector WorldDoorPointB = Transform.TransformPosition(LocalDoorPointB);

			FVector2D CanvasDoorPointA = View.WorldToCanvasLocation(WorldDoorPointA);
			FVector2D CanvasDoorPointB = View.WorldToCanvasLocation(WorldDoorPointB);

			Canvas->K2_DrawLine(CanvasDoorPointA, CanvasDoorPointB, 2);
		}
	}
	
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(InWorld, RenderContext);
	DynamicOcclusionTexture->Modify();
}

void FDungeonCanvasRenderingLibrary::BlurTexture3x(UCanvasRenderTarget2D* SourceTexture, UCanvasRenderTarget2D* DestinationTexture) {
	DungeonCanvasRenderingLib::TBlurTexture<FDAConvGaussBlur3x3Shader>(SourceTexture, DestinationTexture);
}

void FDungeonCanvasRenderingLibrary::BlurTexture5x(UCanvasRenderTarget2D* SourceTexture, UCanvasRenderTarget2D* DestinationTexture) {
	DungeonCanvasRenderingLib::TBlurTexture<FDAConvGaussBlur5x5Shader>(SourceTexture, DestinationTexture);
}

void FDungeonCanvasRenderingLibrary::SetDungeonCanvasMaterialWorldBounds(UMaterialInstanceDynamic* Material, const FTransform& WorldBoundsTransform) {
	if (Material) {
		static const FName ParamWorldCenterX(TEXT("CanvasWorldCenterX"));
		static const FName ParamWorldCenterY(TEXT("CanvasWorldCenterY"));
		static const FName ParamWorldSize(TEXT("CanvasWorldSize"));
		const FVector WorldCenter = WorldBoundsTransform.GetLocation();
		const FVector WorldSize = WorldBoundsTransform.GetScale3D();
		UDungeonCanvasBPFunctionLib::SetDungeonCanvasMaterialScalarParameter(Material, ParamWorldCenterX, WorldCenter.X);
		UDungeonCanvasBPFunctionLib::SetDungeonCanvasMaterialScalarParameter(Material, ParamWorldCenterY, WorldCenter.Y);
		UDungeonCanvasBPFunctionLib::SetDungeonCanvasMaterialScalarParameter(Material, ParamWorldSize, FMath::Max(WorldSize.X, WorldSize.Y));
	}
}


void FDungeonCanvasRenderingLibrary::DungeonCanvasDrawResource(UCanvas* Canvas, const FDungeonCanvasViewportTransform& ViewTransform,
		const FTransform& WorldBoundsTransform, EDungeonCanvasDrawMaterialBlendMode BlendMode, const TFunction<void(FCanvasTriangleItem& Triangle)>& FuncSetResourceObject) {
	if (Canvas && Canvas->Canvas) {
		static const FVector2D QuadUV[] = {
			FVector2D(0, 0),
			FVector2D(1, 0),
			FVector2D(1, 1),
			FVector2D(0, 1),
		};
		
		const FVector WorldBoundsSize = WorldBoundsTransform.GetScale3D();
		const FVector WorldBoundsMin = WorldBoundsTransform.GetLocation() - WorldBoundsSize * 0.5f;
		const FVector WorldBoundsMax = WorldBoundsTransform.GetLocation() + WorldBoundsSize * 0.5f;

		const FVector2D Min = FVector2D(WorldBoundsMin);
		const FVector2D Max = FVector2D(WorldBoundsMax);
		const FVector2D WorldPoints[] = {
			Min,
			FVector2D(Max.X, Min.Y),
			Max,
			FVector2D(Min.X, Max.Y),
		};
		
		const FTransform WorldToCanvas = ViewTransform.GetWorldToCanvas();
		TArray<FVector2D, TFixedAllocator<4>> QuadPoints;
		for (int i = 0; i < 4; i++) {
			FVector CanvasLocation = WorldToCanvas.TransformPosition(FVector(WorldPoints[i], 0));
			QuadPoints.Add(FVector2D(CanvasLocation));
		}

		auto DrawTriangle = [&](int I0, int I1, int I2) {
			FCanvasTriangleItem Triangle(
				QuadPoints[I0], QuadPoints[I1], QuadPoints[I2],
				QuadUV[I0], QuadUV[I1], QuadUV[I2],
				nullptr);

			FuncSetResourceObject(Triangle);

			if (BlendMode == EDungeonCanvasDrawMaterialBlendMode::Opaque) {
				Triangle.BlendMode = SE_BLEND_Opaque;
			}
			else if (BlendMode == EDungeonCanvasDrawMaterialBlendMode::Translucent) {
				Triangle.BlendMode = SE_BLEND_Translucent;
			}
			else if (BlendMode == EDungeonCanvasDrawMaterialBlendMode::AlphaWriteOnly) {
				Triangle.BlendMode = SE_BLEND_TranslucentAlphaOnlyWriteAlpha;
			}
			else if (BlendMode == EDungeonCanvasDrawMaterialBlendMode::AlphaHoldout) {
				Triangle.BlendMode = SE_BLEND_AlphaHoldout;
			}
			
			Canvas->DrawItem(Triangle);
		};

		DrawTriangle(0, 1, 2);
		DrawTriangle(0, 2, 3);
	}
}

void FDungeonCanvasRenderingLibrary::DungeonCanvasDrawMaterial(UCanvas* Canvas, FDungeonCanvasViewportTransform ViewTransform, UMaterialInterface* Material, const FTransform& WorldBoundsTransform, EDungeonCanvasDrawMaterialBlendMode BlendMode) {
	if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Material)) {
		SetDungeonCanvasMaterialWorldBounds(MID, WorldBoundsTransform);
	}
	
	if (Material && Canvas && Canvas->Canvas) {
		// This is a user-facing function, so we'd rather make sure that shaders are ready by the time we render, in order to ensure we don't draw with a fallback material :
		Material->EnsureIsComplete();

		DungeonCanvasDrawResource(Canvas, ViewTransform, WorldBoundsTransform, BlendMode, [&Material](FCanvasTriangleItem& Triangle) {
			Triangle.MaterialRenderProxy = Material->GetRenderProxy();
		});
	}
}

void FDungeonCanvasRenderingLibrary::DungeonCanvasDrawTexture(UCanvas* Canvas, FDungeonCanvasViewportTransform ViewTransform, UTexture* Texture,
		const FTransform& WorldBoundsTransform, EDungeonCanvasDrawMaterialBlendMode BlendMode) {

	FTexture* TextureResource = Texture ? Texture->GetResource() : nullptr;
	if (TextureResource && Canvas && Canvas->Canvas) {
		DungeonCanvasDrawResource(Canvas, ViewTransform, WorldBoundsTransform, BlendMode, [&TextureResource](FCanvasTriangleItem& Triangle) {
			Triangle.Texture = TextureResource;
		});
	}
}

void FDungeonCanvasRenderingLibrary::DungeonCanvasDrawLayoutIcons(UCanvas* Canvas, FDungeonCanvasLayoutRenderResources& RenderResources, const TFunction<void(UMaterialInstanceDynamic* MID)>& FuncSetupMaterial,
			FDungeonCanvasViewportTransform ViewTransform, const FDungeonLayoutHeightRange& HeightRange, const FDungeonLayoutData& DungeonLayout,
			const TArray<FDungeonCanvasOverlayIcon>& OverlayIcons, float OpacityMultiplier)
{
	if (Canvas) {
		TMap<FName, FDungeonCanvasOverlayIcon> OverlayIconMap;
		for (const FDungeonCanvasOverlayIcon& OverlayIcon : OverlayIcons) {
			if (OverlayIcon.ResourceObject && !OverlayIconMap.Contains(OverlayIcon.Name)) {
				OverlayIconMap.Add(OverlayIcon.Name, OverlayIcon);
			}
		}

		const float MinHeight = HeightRange.GetMinHeight();
		const float MaxHeight = HeightRange.GetMaxHeight();
		const FDungeonLayoutData FilteredDungeonLayout = FDungeonLayoutUtils::FilterByHeightRange(DungeonLayout, MinHeight, MaxHeight); 

		const FTransform WorldToCanvas = ViewTransform.GetWorldToCanvas();
		const float WorldToCanvasRotation = WorldToCanvas.GetRotation().Euler().Z;
		
		for (const FDungeonLayoutDataChunkInfo& ChunkShape : FilteredDungeonLayout.ChunkShapes) {
			for (const FDungeonPointOfInterest& PointOfInterest : ChunkShape.PointsOfInterest) {
				FVector Location = PointOfInterest.Transform.GetLocation();
				constexpr float EPSILON = 1e-4f;
				if (Location.Z + EPSILON >= MinHeight && Location.Z - EPSILON <= MaxHeight) {
					if (const FDungeonCanvasOverlayIcon* SearchResult = OverlayIconMap.Find(PointOfInterest.Id)) {
						const FDungeonCanvasOverlayIcon& OverlayIcon = *SearchResult;
						float Rotation = OverlayIcon.RotationOffset;
						if (!OverlayIcon.bAbsoluteRotation) {
							Rotation += WorldToCanvasRotation;
						}
						
						const FVector2D CanvasLocation = ViewTransform.WorldToCanvasLocation(PointOfInterest.Transform.GetLocation());
						float ScreenSize = OverlayIcon.ScreenSize;
						if (OverlayIcon.ScreenSizeType == EDungeonCanvasIconCoordinateSystem::Pixels) {
							ScreenSize = OverlayIcon.ScreenSize;
						}
						else if (OverlayIcon.ScreenSizeType == EDungeonCanvasIconCoordinateSystem::WorldCoordinates) {
							FVector WorldToCanvasScale = ViewTransform.GetWorldToCanvas().GetScale3D() * OverlayIcon.ScreenSize;
							ScreenSize = FMath::Max(WorldToCanvasScale.X, WorldToCanvasScale.Y);
						}

						FLinearColor Tint = OverlayIcon.Tint;
						Tint.A *= OpacityMultiplier;

						DungeonCanvasRenderingLib::DrawCanvasIcon(Canvas, RenderResources, FuncSetupMaterial,
								EDungeonCanvasIconFogOfWarRenderMode::ExploredAndVisibleOnly,
								OverlayIcon.ResourceObject, CanvasLocation, ScreenSize, OverlayIcon.AspectRatio, Tint, Rotation);
					}
				}
			} 
		}
	}
}

void FDungeonCanvasRenderingLibrary::DungeonCanvasDrawStairIcons(UCanvas* Canvas, FDungeonCanvasLayoutRenderResources& RenderResources, const TFunction<void(UMaterialInstanceDynamic* MID)>& FuncSetupMaterial, FDungeonCanvasViewportTransform ViewTransform, const FDungeonLayoutHeightRange& HeightRange,
		const FDungeonLayoutData& DungeonLayout, const FDungeonCanvasOverlayInternalIcon& InStairIcon, float OpacityMultiplier)
{	
	if (Canvas) {
		const FTransform WorldToCanvas = ViewTransform.GetWorldToCanvas();
		const float WorldToCanvasRotation = WorldToCanvas.GetRotation().Euler().Z;
		
		TMap<FName, FDungeonCanvasOverlayIcon> OverlayIconMap;
		const float MinHeight = HeightRange.GetMinHeight();
		const float MaxHeight = HeightRange.GetMaxHeight();
		const FDungeonLayoutData FilteredDungeonLayout = FDungeonLayoutUtils::FilterByHeightRange(DungeonLayout, MinHeight, MaxHeight);

		if (UObject* StairResourceObject = InStairIcon.ResourceObject.LoadSynchronous()) {
			for (const FDungeonLayoutDataStairItem& StairInfo : FilteredDungeonLayout.Stairs) {
				float Rotation = StairInfo.WorldTransform.GetRotation().Euler().Z - 90;
				if (!InStairIcon.bAbsoluteRotation) {
					Rotation += WorldToCanvasRotation;
				}
						
				const FVector2D CanvasLocation = ViewTransform.WorldToCanvasLocation(StairInfo.WorldTransform.GetLocation());
				FLinearColor Tint = InStairIcon.Tint;
				Tint.A *= OpacityMultiplier;
				float LocalWorldSize = StairInfo.Width * InStairIcon.Scale;
				const FVector IconWorldToCanvasScale = ViewTransform.GetWorldToCanvas().GetScale3D() * LocalWorldSize;
				float ScreenSize = FMath::Max(IconWorldToCanvasScale.X, IconWorldToCanvasScale.Y);
			
				DungeonCanvasRenderingLib::DrawCanvasIcon(Canvas, RenderResources, FuncSetupMaterial,
					EDungeonCanvasIconFogOfWarRenderMode::ExploredAndVisibleOnly,
					StairResourceObject, CanvasLocation, ScreenSize, 1.0f, Tint, Rotation);
			}
		}
	}
}

void FDungeonCanvasRenderingLibrary::DungeonCanvasDrawTrackedActorIcons(UCanvas* Canvas, FDungeonCanvasLayoutRenderResources& RenderResources, const TFunction<void(UMaterialInstanceDynamic* MID)>& FuncSetupMaterial, FDungeonCanvasViewportTransform ViewTransform,
		const TArray<FDungeonCanvasTrackedActorRegistryItem>& InTrackedObjects, bool bFogOfWarEnabled, const FDungeonLayoutHeightRange& HeightRange,
		const TArray<FDungeonCanvasOverlayIcon>& OverlayIcons, float OpacityMultiplier, const TArray<UDungeonCanvasActorIconFilter*>& OverlayActorIconFilters)
{
	TArray<FDungeonCanvasTrackedActorRegistryItem> TrackedObjects = InTrackedObjects;

	// Sort the z-order so important icons can render on top of everything else
	TrackedObjects.Sort([](const FDungeonCanvasTrackedActorRegistryItem& A, const FDungeonCanvasTrackedActorRegistryItem& B) {
		if (A.TrackedComponent.IsValid() && B.TrackedComponent.IsValid()) {
			return A.TrackedComponent->ZOrder < B.TrackedComponent->ZOrder;
		}
		return false;
	});

	TMap<FName, FDungeonCanvasOverlayIcon> OverlayIconMap;
	for (const FDungeonCanvasOverlayIcon& OverlayItem : OverlayIcons) {
		if (OverlayItem.ResourceObject && !OverlayIconMap.Contains(OverlayItem.Name)) {
			OverlayIconMap.Add(OverlayItem.Name, OverlayItem);
		}
	}

	if (Canvas) {
		const FTransform WorldToCanvas = ViewTransform.GetWorldToCanvas();
		const float WorldToCanvasRotation = WorldToCanvas.GetRotation().Euler().Z;
		for (const FDungeonCanvasTrackedActorRegistryItem& TrackedItem : TrackedObjects) {
			if (TrackedItem.TrackedComponent.IsValid()) {
				const UDungeonCanvasTrackedObject* TrackedObject = TrackedItem.TrackedComponent.Get();
				if (HeightRange.GetRangeType() != FDungeonLayoutHeightRangeType::EntireDungeon) {
					const float ActorZ = TrackedObject->GetComponentLocation().Z;
					const float MinHeight = HeightRange.GetMinHeight();
					const float MaxHeight = HeightRange.GetMaxHeight();
					if (ActorZ < MinHeight || ActorZ > MaxHeight) {
						continue;
					} 
				}

				/*
				// Check if we need to hide the object based on its fog of war state
				if (TrackedItem.TrackedComponent->bHideWhenOutOfSight && bFogOfWarEnabled) {
					constexpr float VisibilityThreshold = 0.01f;
					if (TrackedItem.FogOfWarLineOfSight < VisibilityThreshold || TrackedItem.FogOfWarExplored < VisibilityThreshold) {
						continue;
					}
				}
				*/
				
				FVector WorldLocation = TrackedObject->GetComponentLocation();
				FVector2D CanvasLocation = ViewTransform.WorldToCanvasLocation(WorldLocation);

				if (FDungeonCanvasOverlayIcon* SearchResult = OverlayIconMap.Find(TrackedObject->IconName)) {
					const FDungeonCanvasOverlayIcon& OverlayData = *SearchResult;
					float Rotation = OverlayData.RotationOffset;
					if (TrackedObject->bOrientToRotation) {
						const FVector Angles = TrackedObject->GetComponentToWorld().GetRotation().Euler();
						Rotation += Angles.Z;
						Rotation += WorldToCanvasRotation;
					}
					float ScreenSize = OverlayData.ScreenSize;
					if (OverlayData.ScreenSizeType == EDungeonCanvasIconCoordinateSystem::Pixels) {
						ScreenSize = OverlayData.ScreenSize;
					}
					else if (OverlayData.ScreenSizeType == EDungeonCanvasIconCoordinateSystem::WorldCoordinates) {
						const FVector WorldToCanvasScale = ViewTransform.GetWorldToCanvas().GetScale3D() * OverlayData.ScreenSize;
						ScreenSize = FMath::Max(WorldToCanvasScale.X, WorldToCanvasScale.Y);
					}
					
					FLinearColor Tint = OverlayData.Tint;
					Tint.A *= OpacityMultiplier;

					// Run this through a filter to modify the transform and looks
					FVector2D CanvasSize(Canvas->SizeX, Canvas->SizeY);
					TObjectPtr<UObject> ResourceObject = OverlayData.ResourceObject;
					for (UDungeonCanvasActorIconFilter* DrawFilter : OverlayActorIconFilters) {
						if (DrawFilter) {
							DrawFilter->ApplyFilter(ViewTransform, OverlayIconMap, OverlayData.Tags, ResourceObject, CanvasSize, CanvasLocation, ScreenSize, Tint, Rotation);
						}
					}

					EDungeonCanvasIconFogOfWarRenderMode FogRenderMode = TrackedObject->bHideWhenOutOfSight
							? EDungeonCanvasIconFogOfWarRenderMode::VisibleOnly
							: EDungeonCanvasIconFogOfWarRenderMode::ExploredAndVisibleOnly;
					
					DungeonCanvasRenderingLib::DrawCanvasIcon(Canvas, RenderResources, FuncSetupMaterial, FogRenderMode, ResourceObject, CanvasLocation, ScreenSize, OverlayData.AspectRatio, Tint, Rotation);
				}
			}
		}
	}
}

UDungeonCanvasThemeRenderResources* FDungeonCanvasRenderingLibrary::CreateDungeonCanvasRenderSettings(UObject* OuterOwner, UDungeonCanvasMaterialTheme* MaterialTheme) {
	if (MaterialTheme) {
		UDungeonCanvasThemeRenderResources* Settings = NewObject<UDungeonCanvasThemeRenderResources>(OuterOwner);
		Settings->ThemeMaterialInstance = UMaterialInstanceDynamic::Create(MaterialTheme->CompiledThemeMaterial, Settings);
		if (UMaterialInterface* FogOfWarMaterialTemplate = MaterialTheme->FogOfWarMaterialTemplate.LoadSynchronous()) {
			Settings->FogOfWarMaterialInstance = UMaterialInstanceDynamic::Create(FogOfWarMaterialTemplate, Settings);
		}
		return Settings;
	}
	return nullptr;
}

void FDungeonCanvasRenderingLibrary::SetupMaterialFloorTextures(UMaterialInstanceDynamic* ThemeMaterial, UMaterialInstanceDynamic* FogOfWarMaterial, const FDungeonCanvasLayoutFloorTextures& FloorTextures) {
	static const FName ParamNameSDF = TEXT("SDF");
	static const FName ParamNameLayoutMask = TEXT("LayoutMask");
	static const FName ParamNameLayoutBorder = TEXT("LayoutBorder");
	static const FName ParamNameFogOfWarExploredTex = TEXT("FogOfWarExploredTex");
	static const FName ParamNameFogOfWarVisibilityTex = TEXT("FogOfWarVisibilityTex");
	
	UDungeonCanvasBPFunctionLib::SetDungeonCanvasMaterialTextureParameter(ThemeMaterial, ParamNameSDF, FloorTextures.SDF);
	UDungeonCanvasBPFunctionLib::SetDungeonCanvasMaterialTextureParameter(ThemeMaterial, ParamNameLayoutMask, FloorTextures.LayoutFill);
	UDungeonCanvasBPFunctionLib::SetDungeonCanvasMaterialTextureParameter(ThemeMaterial, ParamNameLayoutBorder, FloorTextures.LayoutBorder);
	UDungeonCanvasBPFunctionLib::SetDungeonCanvasMaterialTextureParameter(ThemeMaterial, ParamNameFogOfWarExploredTex, FloorTextures.FogOfWarExplored);
	UDungeonCanvasBPFunctionLib::SetDungeonCanvasMaterialTextureParameter(ThemeMaterial, ParamNameFogOfWarVisibilityTex, FloorTextures.FogOfWarVisibility);
	if (FogOfWarMaterial) {
		FogOfWarMaterial->SetTextureParameterValue(ParamNameFogOfWarExploredTex, FloorTextures.FogOfWarExplored);
		FogOfWarMaterial->SetTextureParameterValue(ParamNameFogOfWarVisibilityTex, FloorTextures.FogOfWarVisibility);
	}
}

void FDungeonCanvasRenderingLibrary::SetupMaterialFogOfWarState(UMaterialInstanceDynamic* ThemeMaterial, UMaterialInstanceDynamic* FogOfWarMaterial, bool bFogOfWarEnabled, bool bFullyExplored) {
	static const FName ParamNameFullyExplored = TEXT("FullyExplored");
	static const FName ParamNameFogOfWarEnabled = TEXT("FogOfWarEnabled");
	
	// Assign the fog of war settings
	UDungeonCanvasBPFunctionLib::SetDungeonCanvasMaterialScalarParameter(ThemeMaterial, ParamNameFullyExplored, bFullyExplored ? 1.0f : 0.0f);
	UDungeonCanvasBPFunctionLib::SetDungeonCanvasMaterialScalarParameter(ThemeMaterial, ParamNameFogOfWarEnabled, bFogOfWarEnabled ? 1.0f : 0.0f);
	if (FogOfWarMaterial) {
		FogOfWarMaterial->SetScalarParameterValue(ParamNameFullyExplored, bFullyExplored ? 1.0f : 0.0f);
		FogOfWarMaterial->SetScalarParameterValue(ParamNameFogOfWarEnabled, bFogOfWarEnabled ? 1.0f : 0.0f);
	}
}

void FDungeonCanvasRenderingLibrary::BeginFogOfWarUpdate(UCanvasRenderTarget2D* FogOfWarExploredTexture, UCanvasRenderTarget2D* FogOfWarVisibilityTexture) {
	if (!FogOfWarExploredTexture || !FogOfWarVisibilityTexture) {
		return;
	}
	if (FogOfWarExploredTexture->SizeX != FogOfWarVisibilityTexture->SizeX || FogOfWarExploredTexture->SizeY != FogOfWarVisibilityTexture->SizeY) {
		return;
	}
	
	const int32 SurfaceWidth = FogOfWarExploredTexture->SizeX;
	const int32 SurfaceHeight = FogOfWarExploredTexture->SizeY;

	if (SurfaceWidth == 0 || SurfaceHeight == 0) {
		return;
	}

	const FTextureResource* FoWVisibilityResource = FogOfWarVisibilityTexture->GetResource();

	const FIntVector ThreadGroupsFog = FIntVector(
			FMath::DivideAndRoundUp(SurfaceWidth, 16),
			FMath::DivideAndRoundUp(SurfaceHeight, 16),
			1);
	
	ENQUEUE_RENDER_COMMAND(DAUpdateFogOfWarInit)(
		[=](FRHICommandListImmediate& RHICmdList) {
			FRDGBuilder GraphBuilder(RHICmdList);

			TRefCountPtr<IPooledRenderTarget> FogTexVisibilityCache;
			CacheRenderTarget(FoWVisibilityResource->TextureRHI, TEXT("FoW_FogVisibilityTex"), FogTexVisibilityCache);

			const FRDGTextureRef FogTexVisibilityRDG = GraphBuilder.RegisterExternalTexture(FogTexVisibilityCache);

			FDACanvasFogOfWarInitFrameShader::FParameters* FogOfWarParams = GraphBuilder.AllocParameters<FDACanvasFogOfWarInitFrameShader::FParameters>();
			FogOfWarParams->TexFogOfWarVisibility = GraphBuilder.CreateUAV(FogTexVisibilityRDG);

			const TShaderMapRef<FDACanvasFogOfWarInitFrameShader> FogOfWarShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("CanvasFogOfWarInit"), FogOfWarShader, FogOfWarParams, ThreadGroupsFog);

			GraphBuilder.Execute();
		});

	FogOfWarExploredTexture->Modify();
	FogOfWarVisibilityTexture->Modify();
}

void FDungeonCanvasRenderingLibrary::UpdateFogOfWarExplorer(UCanvasRenderTarget2D* FogOfWarExploredTexture, UCanvasRenderTarget2D* FogOfWarVisibilityTexture,
		UTexture* SDFTexture, const FDungeonCanvasViewportTransform& FullDungeonTransform, const FVector2D& LightSourceLocation,
		float LightRadius, float NumShadowSamples, int ShadowJitterDistance)
{

	NumShadowSamples = FMath::Clamp(NumShadowSamples, 1, 10);

	int32 SurfaceWidth{}, SurfaceHeight{};
	{
		if (const UTextureRenderTarget2D* RTT = Cast<UTextureRenderTarget2D>(SDFTexture)) {
			SurfaceWidth = RTT->SizeX;
			SurfaceHeight = RTT->SizeY;
		}
		else if (const UTexture2D* Tex2D = Cast<UTexture2D>(SDFTexture)) {
			SurfaceWidth = Tex2D->GetSizeX();
			SurfaceHeight = Tex2D->GetSizeY();
		}
	}

	auto IsTexValid = [SurfaceWidth, SurfaceHeight](const UCanvasRenderTarget2D* Texture) {
		return Texture != nullptr && Texture->SizeX == SurfaceWidth && Texture->SizeY == SurfaceHeight; 
	};
	
	if (SurfaceWidth == 0 || SurfaceHeight == 0 || !IsTexValid(FogOfWarExploredTexture) || !IsTexValid(FogOfWarVisibilityTexture)) {
		return;
	}

	const FTextureResource* SDFResource = SDFTexture->GetResource();
	const FTextureResource* FoWExploredResource = FogOfWarExploredTexture->GetResource();
	const FTextureResource* FoWVisibilityResource = FogOfWarVisibilityTexture->GetResource();

	const FIntPoint TextureSize(SurfaceWidth, SurfaceHeight);

	const FTransform WorldBounds = FullDungeonTransform.GetLocalToWorld();
	const FVector WorldBoundsSize = WorldBounds.GetScale3D();
	const FVector WorldBoundsMin = WorldBounds.GetLocation() - WorldBoundsSize * 0.5f;
	const FVector WorldBoundsMax = WorldBounds.GetLocation() + WorldBoundsSize * 0.5f;
	
	const FVector2D LocationUV = (LightSourceLocation - FVector2D(WorldBoundsMin)) / FVector2D(WorldBoundsSize);
	auto Round2D = [](const FVector2D& V) {
		return FIntPoint(FMath::RoundToInt(V.X), FMath::RoundToInt(V.Y));
	};

	const FIntPoint FoWSourcePixel = Round2D(LocationUV * TextureSize);
	const float RadiusUV = LightRadius / WorldBoundsSize.X;
	const float RadiusPixels = RadiusUV * SurfaceWidth;

	const float SoftShadowRadiusPixels = ShadowJitterDistance / WorldBoundsSize.X * SurfaceWidth;

	const FIntPoint BaseOffset = FoWSourcePixel - FIntPoint(RadiusPixels, RadiusPixels);	
	const FIntPoint BaseSize = FIntPoint(RadiusPixels, RadiusPixels) * 2;

	const FIntVector ThreadGroupsFog = FIntVector(
			FMath::DivideAndRoundUp(BaseSize.X, 16),
			FMath::DivideAndRoundUp(BaseSize.Y, 16),
			1);

	ENQUEUE_RENDER_COMMAND(DAUpdateFogOfWar)(
		[=](FRHICommandListImmediate& RHICmdList) {
			FRDGBuilder GraphBuilder(RHICmdList);

			TRefCountPtr<IPooledRenderTarget> FogTexExploredCache, FogTexVisibilityCache, SDFTexCache;
			CacheRenderTarget(FoWExploredResource->TextureRHI, TEXT("FoW_FogExploredTex"), FogTexExploredCache);
			CacheRenderTarget(FoWVisibilityResource->TextureRHI, TEXT("FoW_FogVisibilityTex"), FogTexVisibilityCache);
			CacheRenderTarget(SDFResource->TextureRHI, TEXT("FoW_SDFTex"), SDFTexCache);
			
			const FRDGTextureRef FogTexExploredRDG = GraphBuilder.RegisterExternalTexture(FogTexExploredCache);
			const FRDGTextureRef FogTexVisibilityRDG = GraphBuilder.RegisterExternalTexture(FogTexVisibilityCache);
			const FRDGTextureRef SDFTexRDG = GraphBuilder.RegisterExternalTexture(SDFTexCache);

			FDACanvasFogOfWarShader::FParameters* FogOfWarParams = GraphBuilder.AllocParameters<FDACanvasFogOfWarShader::FParameters>();
			FogOfWarParams->FoWSourcePixel = FoWSourcePixel;
			FogOfWarParams->BaseOffset = BaseOffset;
			FogOfWarParams->RadiusPixels = RadiusPixels;

			FogOfWarParams->SoftShadowRadius = SoftShadowRadiusPixels;
			FogOfWarParams->SoftShadowSamples = NumShadowSamples;
			
			FogOfWarParams->TextureWidth = SurfaceWidth;
			FogOfWarParams->TextureHeight = SurfaceHeight;
			FogOfWarParams->TexSDF = GraphBuilder.CreateSRV(SDFTexRDG);
			FogOfWarParams->TexFogOfWarExplored = GraphBuilder.CreateUAV(FogTexExploredRDG);
			FogOfWarParams->TexFogOfWarVisibility = GraphBuilder.CreateUAV(FogTexVisibilityRDG);

			const TShaderMapRef<FDACanvasFogOfWarShader> FogOfWarShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("CanvasFogOfWar"), FogOfWarShader, FogOfWarParams, ThreadGroupsFog);

			GraphBuilder.Execute();
		});
	
	FogOfWarExploredTexture->Modify();
	FogOfWarVisibilityTexture->Modify();
}

float FDungeonCanvasRenderingLibrary::ConvertLocalSDFValueToWorldUnits(float NormalizedPixelSDFValue, const FVector& WorldBoundsScale) {
	return NormalizedPixelSDFValue * FMath::Max(WorldBoundsScale.X, WorldBoundsScale.Y);
}

void FDungeonCanvasRenderingLibrary::SetupMaterialParameters(UMaterialInstanceDynamic* MID, const FDungeonCanvasLayoutFloorTextures& FloorTextures,
		const FDungeonCanvasViewportTransform& ViewTransform, const FDungeonCanvasViewportTransform& FullDungeonTransform)
{
	FVector ViewportWorldLocMid = ViewTransform.CanvasToWorldLocation(ViewTransform.UVToCanvas(FVector2D(0.5f, 0.5f)));
	FVector2D FullCanvasUVMid = FullDungeonTransform.CanvasToUV(FullDungeonTransform.WorldToCanvasLocation(ViewportWorldLocMid));

	FVector ViewportScale3D = ViewTransform.GetLocalToWorld().GetScale3D() / FullDungeonTransform.GetLocalToWorld().GetScale3D();
	FVector::FReal ViewportScale = FMath::Max(ViewportScale3D.X, ViewportScale3D.Y);	// TODO: Check if we need min, instead of max
					
	static const FName ParamNameFogOfWarExploredTex = TEXT("FogOfWarExploredTex");
	static const FName ParamNameFogOfWarVisibilityTex = TEXT("FogOfWarVisibilityTex");
					
	FVector::FReal Rotation = ViewTransform.GetCanvasToWorld().Rotator().Yaw;
	Rotation /= 360.0f;
	if (Rotation < 0) {
		Rotation += 1;
	}

	if (MID) {
		MID->SetTextureParameterValue(ParamNameFogOfWarExploredTex, FloorTextures.FogOfWarExplored);
		MID->SetTextureParameterValue(ParamNameFogOfWarVisibilityTex, FloorTextures.FogOfWarVisibility);

		static const FName ParamNameViewportUVMid = TEXT("ViewportUVMid");
		static const FName ParamNameViewportUVSize = TEXT("ViewportUVSize");
		static const FName ParamNameViewportUVRot = TEXT("ViewportUVRot");
		MID->SetVectorParameterValue(ParamNameViewportUVMid, FVector(FullCanvasUVMid, 0));
		MID->SetVectorParameterValue(ParamNameViewportUVSize, ViewportScale3D);
		MID->SetScalarParameterValue(ParamNameViewportUVRot, Rotation);
	}
}


void FDungeonCanvasRenderingLibrary::GenerateFloorTextures(UWorld* World, UObject* Owner, const FDungeonLayoutData& InDungeonLayout, int32 LayoutTextureSize,
			float PaddingWorldUnits, bool bDrawAllFloors, int32 FloorIndex, FDungeonCanvasLayoutFloorTextures& OutFloorTextures, const FFloorTexturesGenerationOptions& InOptions)
{
	FFloorTexturesGenerationOptions Options = InOptions;
	if (Options.bGenerateSDF) {
		Options.bGenerateLayoutFill = true;
		Options.bGenerateLayoutBorder = true;
	}
	
	// Create the floor texture objects in memory, so we can later write to it
	CreateDungeonCanvasLayoutFloorTextures(Owner, LayoutTextureSize, LayoutTextureSize, OutFloorTextures, Options);

	DrawDungeonLayout(World, InDungeonLayout, PaddingWorldUnits, bDrawAllFloors, FloorIndex, OutFloorTextures.GroundHeightBias, OutFloorTextures.LayoutFill,
		OutFloorTextures.LayoutBorder, OutFloorTextures.GroundHeightMin, OutFloorTextures.GroundHeightMax);

	// Draw the SDF texture
	GenerateDungeonSDF(OutFloorTextures.LayoutFill, OutFloorTextures.LayoutBorder, OutFloorTextures.DynamicOcclusion, OutFloorTextures.SDF);
}

void FDungeonCanvasRenderingLibrary::GenerateDungeonSDF(UTexture* FillTexture, UTexture* BorderTexture, UTexture* DynamicOcclusionTexture, UCanvasRenderTarget2D* SDFTexture) {
	if (!FillTexture || !SDFTexture) {
		return;
	}
	if (FillTexture->GetSurfaceWidth() != SDFTexture->GetSurfaceWidth()
			|| FillTexture->GetSurfaceHeight() != SDFTexture->GetSurfaceHeight()) {
		return;
	}
  
	FTexture* MaskResource = FillTexture->GetResource();
	FTexture* BorderResource = BorderTexture->GetResource();
	FTexture* SDFResource = SDFTexture->GetResource();
	FTexture* DynamicOcclusionResource = DynamicOcclusionTexture ? DynamicOcclusionTexture->GetResource() : GBlackTexture;
	if (!MaskResource || !SDFResource || !DynamicOcclusionResource) {
		return;
	}
	
	const int32 SurfaceWidth = FillTexture->GetSurfaceWidth();
	const int32 SurfaceHeight = FillTexture->GetSurfaceHeight();

	SDFTexture->Modify();
	ENQUEUE_RENDER_COMMAND(DAGenerateSDF)(
		[MaskResource, BorderResource, SDFResource, DynamicOcclusionResource, SurfaceWidth, SurfaceHeight]
			(FRHICommandListImmediate& RHICmdList){
		//FMemMark MemMark(FMemStack::Get());
		FRDGBuilder GraphBuilder(RHICmdList);


		TRefCountPtr<IPooledRenderTarget> MaskTexCache, BorderTexCache, SDFTexCache, OcclusionTexCache;
		CacheRenderTarget(MaskResource->TextureRHI, TEXT("GenSDF_MaskTex"), MaskTexCache);
		CacheRenderTarget(BorderResource->TextureRHI, TEXT("GenSDF_BorderTex"), BorderTexCache);
		CacheRenderTarget(SDFResource->TextureRHI, TEXT("GenSDF_SDFTex"), SDFTexCache);
		CacheRenderTarget(DynamicOcclusionResource->TextureRHI, TEXT("GenSDF_OcclusionTex"), OcclusionTexCache);
			
			
		const FRDGTextureRef MaskTexRDG = GraphBuilder.RegisterExternalTexture(MaskTexCache);
		const FRDGTextureRef BorderTexRDG = GraphBuilder.RegisterExternalTexture(BorderTexCache);
		const FRDGTextureRef SDFTexRDG = GraphBuilder.RegisterExternalTexture(SDFTexCache);
		const FRDGTextureRef OcclusionTexRDG = GraphBuilder.RegisterExternalTexture(OcclusionTexCache);

		const FRDGTextureSRVRef MaskTexSRV = GraphBuilder.CreateSRV(MaskTexRDG);
		const FRDGTextureSRVRef BorderTexSRV = GraphBuilder.CreateSRV(BorderTexRDG);
		const FRDGTextureUAVRef SDFTexUAV = GraphBuilder.CreateUAV(SDFTexRDG);
		const FRDGTextureSRVRef OcclusionTexSRV = GraphBuilder.CreateSRV(OcclusionTexRDG);
			
		FRDGBufferUAVRef NearestPointBufferUAV;

		{
			const int32 NumBufferElements = SurfaceWidth * SurfaceHeight;
			FRDGBufferDesc BufferDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector2f), NumBufferElements);
			BufferDesc.Usage |= BUF_SourceCopy | BUF_UnorderedAccess;
			const FRDGBufferRef NearestPointBuffer = GraphBuilder.CreateBuffer(BufferDesc, TEXT("GenSDF_NearestPointBuf"));
			NearestPointBufferUAV = GraphBuilder.CreateUAV(NearestPointBuffer);
		}

		const int32 ThreadGroupsPerSideX = FMath::DivideAndRoundUp(SurfaceWidth, 16);
		const int32 ThreadGroupsPerSideY = FMath::DivideAndRoundUp(SurfaceHeight, 16);
		const FIntVector ThreadGroups(ThreadGroupsPerSideX, ThreadGroupsPerSideY, 1);
			
		// Initialize the buffers
		{
			FDAJFAInitShader::FParameters* JFAParams = GraphBuilder.AllocParameters<FDAJFAInitShader::FParameters>();
			JFAParams->TextureWidth = SurfaceWidth;
			JFAParams->TextureHeight = SurfaceHeight;
			JFAParams->NearestPoint = NearestPointBufferUAV;
			JFAParams->MaskTexture = MaskTexSRV;
			JFAParams->BorderTexture = BorderTexSRV;
			JFAParams->OcclusionTexture = OcclusionTexSRV;
			JFAParams->SDFTexture = SDFTexUAV;

			const TShaderMapRef<FDAJFAInitShader> JFAShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("JFAInit"), JFAShader, JFAParams, ThreadGroups);
		}
	
		int JumpDistance = SurfaceWidth / 2;
		
		while (JumpDistance >= 1) {
			FDAJFAFindNearestPointShader::FParameters* JFAParams = GraphBuilder.AllocParameters<FDAJFAFindNearestPointShader::FParameters>();
			JFAParams->JumpDistance = JumpDistance;
			JFAParams->TextureWidth = SurfaceWidth;
			JFAParams->TextureHeight = SurfaceHeight;
			JFAParams->MaskTexture = MaskTexSRV;
			JFAParams->NearestPoint = NearestPointBufferUAV;

			const TShaderMapRef<FDAJFAFindNearestPointShader> JFAShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("JFANearestPoint"), JFAShader, JFAParams, ThreadGroups);

			JumpDistance /= 2;
		}

		// Write the result out to the SDF texture
		{
			FDAJFAWriteSDFShader::FParameters* JFAParams = GraphBuilder.AllocParameters<FDAJFAWriteSDFShader::FParameters>();
			JFAParams->TextureWidth = SurfaceWidth;
			JFAParams->TextureHeight = SurfaceHeight;
			JFAParams->MaxSDFValue = FMath::Max(SurfaceWidth, SurfaceHeight);
			JFAParams->MaskTexture = MaskTexSRV;
			JFAParams->NearestPoint = NearestPointBufferUAV;
			JFAParams->SDFTexture = SDFTexUAV;

			const TShaderMapRef<FDAJFAWriteSDFShader> JFAShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("JFAWriteSDF"), JFAShader, JFAParams, ThreadGroups);
		}
		
		GraphBuilder.Execute();
	});

	// Update the resources to regenerate the mips
	if (SDFTexture->bAutoGenerateMips) {
		SDFTexture->UpdateResourceImmediate(false);
	}
}


//////////////////////////// UDungeonCanvasBPFunctionLib ////////////////////////////

void UDungeonCanvasBPFunctionLib::GenerateDungeonSDF(UTexture* FillTexture, UTexture* BorderTexture, UTexture* DynamicOcclusionTexture, UCanvasRenderTarget2D* SDFTexture) {
	FDungeonCanvasRenderingLibrary::GenerateDungeonSDF(FillTexture, BorderTexture, DynamicOcclusionTexture, SDFTexture);
}

void UDungeonCanvasBPFunctionLib::GenerateDungeonVoronoiSdfEffect(UTexture* SDFTexture, UTexture* BorderTexture,
		UCanvasRenderTarget2D* TargetEffectTexture, float ScaleMin, float ScaleMax)
{
	if (!SDFTexture || !TargetEffectTexture || !BorderTexture) {
		return;
	}
	if (TargetEffectTexture->GetSurfaceWidth() != SDFTexture->GetSurfaceWidth()
			|| TargetEffectTexture->GetSurfaceHeight() != SDFTexture->GetSurfaceHeight()) {
		return;
	}
	
	FTexture* SDFResource = SDFTexture->GetResource();
	FTexture* TargetResource = TargetEffectTexture->GetResource();
	FTexture* BorderResource = BorderTexture ? BorderTexture->GetResource() : nullptr;
	if (!SDFResource || !TargetResource) {
		return;
	}
	
	const int32 SurfaceWidth = SDFTexture->GetSurfaceWidth();
	const int32 SurfaceHeight = SDFTexture->GetSurfaceHeight();
	const FIntPoint TextureSize(SurfaceWidth, SurfaceHeight);

	
	TargetEffectTexture->Modify();
	ENQUEUE_RENDER_COMMAND(DAGenerateVoronoiEffect)([SDFResource, TargetResource, BorderResource, SurfaceWidth, SurfaceHeight, TextureSize](FRHICommandListImmediate& RHICmdList){
		FRDGBuilder GraphBuilder(RHICmdList);

		
		TRefCountPtr<IPooledRenderTarget> TargetTexCache, SDFTexCache, BorderTexCache, OcclusionTexCache;
		CacheRenderTarget(SDFResource->TextureRHI, TEXT("GenVoronoi_SDFTex"), SDFTexCache);
		CacheRenderTarget(TargetResource->TextureRHI, TEXT("GenVoronoi_TargetTex"), TargetTexCache);
		CacheRenderTarget(BorderResource->TextureRHI, TEXT("GenVoronoi_BorderTex"), BorderTexCache);
		CacheRenderTarget(GBlackTexture->TextureRHI, TEXT("GenVoronoi_OcclusionTex"), OcclusionTexCache);
		
		const FRDGTextureRef SDFTexRDG = GraphBuilder.RegisterExternalTexture(SDFTexCache);
		const FRDGTextureRef TargetTexRDG = GraphBuilder.RegisterExternalTexture(TargetTexCache);
		const FRDGTextureRef BorderTexRDG = GraphBuilder.RegisterExternalTexture(BorderTexCache);

		const FRDGTextureSRVRef SDFTexSRV = GraphBuilder.CreateSRV(SDFTexRDG);
		const FRDGTextureUAVRef TargetTexUAV = GraphBuilder.CreateUAV(TargetTexRDG);
		const FRDGTextureSRVRef TargetTexSRV = GraphBuilder.CreateSRV(TargetTexRDG);

		constexpr int NumCellsPerSide = 100;
		const int CellSize = FMath::RoundToInt(TargetResource->GetSizeX() / static_cast<float>(NumCellsPerSide));

		// Generate site points based on the sdf density
		TRefCountPtr<IPooledRenderTarget> SitePointTexture; 
		{
			const TCHAR* SitePointTextureName = TEXT("SitePointTexture");
			const FPooledRenderTargetDesc SitePointTexDesc = FPooledRenderTargetDesc::Create2DDesc(
			TextureSize,
			PF_R8,
			FClearValueBinding::None,
			TexCreate_None,
			TexCreate_ShaderResource | TexCreate_RenderTargetable | TexCreate_UAV,
			false);

			GRenderTargetPool.FindFreeElement(RHICmdList, SitePointTexDesc, SitePointTexture, SitePointTextureName);
		}
	
		const FRDGTextureRef SitePointTexRDG = GraphBuilder.RegisterExternalTexture(SitePointTexture);
		FRDGTextureSRVRef SitePointTexSRV = GraphBuilder.CreateSRV(SitePointTexRDG);
		FRDGTextureUAVRef SitePointTexUAV = GraphBuilder.CreateUAV(SitePointTexRDG);
		FRDGTextureSRVRef BorderTexSRV = GraphBuilder.CreateSRV(BorderTexRDG);
		
		AddClearRenderTargetPass(GraphBuilder, SitePointTexRDG, FLinearColor::Black);
		
		{
			FDACanvasVoronoiSDFShader::FParameters* EffectParams = GraphBuilder.AllocParameters<FDACanvasVoronoiSDFShader::FParameters>();
			EffectParams->TextureWidth = SurfaceWidth;
			EffectParams->TextureHeight = SurfaceHeight;
			EffectParams->CellSize = CellSize;
			EffectParams->TexSDF = SDFTexSRV;
			EffectParams->TexVoronoi = SitePointTexUAV;

			const int32 ThreadGroupsPerSideX = FMath::DivideAndRoundUp(NumCellsPerSide, 16);
			const int32 ThreadGroupsPerSideY = FMath::DivideAndRoundUp(NumCellsPerSide, 16);
			const FIntVector ThreadGroups(ThreadGroupsPerSideX, ThreadGroupsPerSideY, 1);
			
			const TShaderMapRef<FDACanvasVoronoiSDFShader> VoronoiSDFShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("VoronoiSDFGen"), VoronoiSDFShader, EffectParams, ThreadGroups);
		}

		const int32 ThreadGroupsPerSideX = FMath::DivideAndRoundUp(SurfaceWidth, 16);
		const int32 ThreadGroupsPerSideY = FMath::DivideAndRoundUp(SurfaceHeight, 16);
		const FIntVector ThreadGroups(ThreadGroupsPerSideX, ThreadGroupsPerSideY, 1);

		/*
		// Copy the border to the sites texture
		{
			TDABlitShader<EDABlurShaderOp::Add>::FParameters* BlitParams = GraphBuilder.AllocParameters<TDABlitShader<EDABlurShaderOp::Add>::FParameters>();
			BlitParams->TexSource = BorderTexSRV;
			BlitParams->TexDest = SitePointTexUAV;
			
			const TShaderMapRef<TDABlitShader<EDABlurShaderOp::Add>> BlitShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("VoronoiSiteBorderCopy"), BlitShader, BlitParams, ThreadGroups);
		}
		*/
		
		// Run JFA sdf shader on the site point texture, to create voronoi
		FRDGBufferUAVRef NearestPointBufferUAV;

		{
			const int32 NumBufferElements = SurfaceWidth * SurfaceHeight;
			FRDGBufferDesc BufferDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector2f), NumBufferElements);
			BufferDesc.Usage |= BUF_SourceCopy | BUF_UnorderedAccess;
			const FRDGBufferRef NearestPointBuffer = GraphBuilder.CreateBuffer(BufferDesc, TEXT("GenSDF_NearestPointBuf"));
			NearestPointBufferUAV = GraphBuilder.CreateUAV(NearestPointBuffer);
		}
		
		// Initialize the buffers
		{
			FDAJFAInitShader::FParameters* JFAParams = GraphBuilder.AllocParameters<FDAJFAInitShader::FParameters>();
			JFAParams->TextureWidth = SurfaceWidth;
			JFAParams->TextureHeight = SurfaceHeight;
			JFAParams->NearestPoint = NearestPointBufferUAV;
			JFAParams->MaskTexture = SitePointTexSRV;
			JFAParams->BorderTexture = SitePointTexSRV;
			JFAParams->OcclusionTexture = SitePointTexSRV;
			JFAParams->SDFTexture = TargetTexUAV;

			const TShaderMapRef<FDAJFAInitShader> JFAShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("JFAInit"), JFAShader, JFAParams, ThreadGroups);
		}
	
		int JumpDistance = SurfaceWidth / 2;
		
		while (JumpDistance >= 1) {
			FDAJFAFindNearestPointShader::FParameters* JFAParams = GraphBuilder.AllocParameters<FDAJFAFindNearestPointShader::FParameters>();
			JFAParams->JumpDistance = JumpDistance;
			JFAParams->TextureWidth = SurfaceWidth;
			JFAParams->TextureHeight = SurfaceHeight;
			JFAParams->MaskTexture = SitePointTexSRV;
			JFAParams->NearestPoint = NearestPointBufferUAV;

			const TShaderMapRef<FDAJFAFindNearestPointShader> JFAShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("JFANearestPoint"), JFAShader, JFAParams, ThreadGroups);

			JumpDistance /= 2;
		}

		// Write the result out to the SDF texture
		{
			FDAJFAWriteSDFEdgeShader::FParameters* JFAParams = GraphBuilder.AllocParameters<FDAJFAWriteSDFEdgeShader::FParameters>();
			JFAParams->TextureWidth = SurfaceWidth;
			JFAParams->TextureHeight = SurfaceHeight;
			JFAParams->NearestPoint = NearestPointBufferUAV;
			JFAParams->SDFTexture = TargetTexUAV;

			const TShaderMapRef<FDAJFAWriteSDFEdgeShader> JFAShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("JFAWriteSDF"), JFAShader, JFAParams, ThreadGroups);
		}

		const int NumBlurIterations = 6;
		for (int i = 0; i < NumBlurIterations; i++) {
			// blur the sdf tex
			{
				FDAConvGaussBlur5x5Shader::FParameters* BlurParams = GraphBuilder.AllocParameters<FDAConvGaussBlur5x5Shader::FParameters>();
				BlurParams->TextureWidth = SurfaceWidth;
				BlurParams->TextureHeight = SurfaceHeight;
				BlurParams->InTexture = TargetTexSRV;
				BlurParams->OutTexture = SitePointTexUAV;

				const TShaderMapRef<FDAConvGaussBlur5x5Shader> BlurShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
				FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("VoroBlur"), BlurShader, BlurParams, ThreadGroups);
			}

			// Blit it back to the target texture
			{
				TDABlitShader<EDABlurShaderOp::Copy>::FParameters* BlitParams = GraphBuilder.AllocParameters<TDABlitShader<EDABlurShaderOp::Copy>::FParameters>();
				BlitParams->TexSource = SitePointTexSRV;
				BlitParams->TexDest = TargetTexUAV;
				const TShaderMapRef<TDABlitShader<EDABlurShaderOp::Copy>> BlitShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
				FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("VoroBlit"), BlitShader, BlitParams, ThreadGroups);
			}
		}
		
		GraphBuilder.Execute();
	});
	TargetEffectTexture->UpdateResourceImmediate(false);
	TargetEffectTexture->Modify();
}

void UDungeonCanvasBPFunctionLib::SetDungeonCanvasMaterialScalarParameter(UMaterialInstanceDynamic* Material, FName ParamName, float Value) {
	if (Material) {
		FMaterialLayersFunctions MaterialLayersFunctions;
		if (Material->GetMaterialLayers(MaterialLayersFunctions)) {
			// Set the global parameter
			Material->SetScalarParameterValue(ParamName, Value);

			// Try setting this parameter in each of the material layers
			const int32 NumLayers = MaterialLayersFunctions.Layers.Num();
			for (int LayerIdx = 0; LayerIdx < NumLayers; LayerIdx++) {
				FMaterialParameterInfo ParamInfo(ParamName, LayerParameter, LayerIdx);
				Material->SetScalarParameterValueByInfo(ParamInfo, Value);
			}
		}
	}
}

void UDungeonCanvasBPFunctionLib::SetDungeonCanvasMaterialVectorParameter(UMaterialInstanceDynamic* Material, FName ParamName, FVector Value) {
	if (Material) {
		FMaterialLayersFunctions MaterialLayersFunctions;
		if (Material->GetMaterialLayers(MaterialLayersFunctions)) {
			// Set the global parameter
			Material->SetVectorParameterValue(ParamName, Value);

			// Try setting this parameter in each of the material layers
			const int32 NumLayers = MaterialLayersFunctions.Layers.Num();
			for (int LayerIdx = 0; LayerIdx < NumLayers; LayerIdx++) {
				FMaterialParameterInfo ParamInfo(ParamName, LayerParameter, LayerIdx);
				Material->SetVectorParameterValueByInfo(ParamInfo, Value);
			}
		}
	}
}

void UDungeonCanvasBPFunctionLib::SetDungeonCanvasMaterialColorParameter(UMaterialInstanceDynamic* Material, FName ParamName, FLinearColor Value) {
	if (Material) {
		FMaterialLayersFunctions MaterialLayersFunctions;
		if (Material->GetMaterialLayers(MaterialLayersFunctions)) {
			// Set the global parameter
			Material->SetVectorParameterValue(ParamName, Value);

			// Try setting this parameter in each of the material layers
			const int32 NumLayers = MaterialLayersFunctions.Layers.Num();
			for (int LayerIdx = 0; LayerIdx < NumLayers; LayerIdx++) {
				FMaterialParameterInfo ParamInfo(ParamName, LayerParameter, LayerIdx);
				Material->SetVectorParameterValueByInfo(ParamInfo, Value);
			}
		}
	}
}

void UDungeonCanvasBPFunctionLib::SetDungeonCanvasMaterialTextureParameter(UMaterialInstanceDynamic* Material, FName ParamName, UTexture* Value) {
	if (Material) {
		FMaterialLayersFunctions MaterialLayersFunctions;
		if (Material->GetMaterialLayers(MaterialLayersFunctions)) {
			// Set the global parameter
			Material->SetTextureParameterValue(ParamName, Value);

			// Try setting this parameter in each of the material layers
			const int32 NumLayers = MaterialLayersFunctions.Layers.Num();
			for (int LayerIdx = 0; LayerIdx < NumLayers; LayerIdx++) {
				FMaterialParameterInfo ParamInfo(ParamName, LayerParameter, LayerIdx);
				Material->SetTextureParameterValueByInfo(ParamInfo, Value);
			}
		}
	}
}

