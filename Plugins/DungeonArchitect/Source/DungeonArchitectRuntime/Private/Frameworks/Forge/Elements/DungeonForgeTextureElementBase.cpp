//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Elements/DungeonForgeTextureElementBase.h"

#include "Core/Data/DungeonSpatialMapData.h"
#include "Core/Dungeon.h"
#include "Core/Utils/DungeonLog.h"
#include "Frameworks/Forge/Common/DungeonForgePoint.h"
#include "Frameworks/Forge/Data/DungeonForgeTextureData.h"
#include "Frameworks/Forge/DungeonForgeResourceManager.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"
#include "Frameworks/Lib/Shaders/ReadbackRTShader.h"

#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "TextureResource.h"

#define LOCTEXT_NAMESPACE "DungeonForgeTextureSettingsBase"

FDungeonForgeTextureDataFrameTransformer::FDungeonForgeTextureDataFrameTransformer(float InBaseScale, int32 InWidth, int32 InHeight, const FTransform& InWorldTransform)
	: BaseScale(InBaseScale)
	, Width(InWidth)
	, Height(InHeight)
	, WorldTransform(InWorldTransform)
{
	LocalScale = FVector(BaseScale, BaseScale, 1) / FVector(Width - 1, Height - 1, 1);
	LocalOffset = FVector(-BaseScale * 0.5f, -BaseScale * 0.5f, 10);
}

FVector FDungeonForgeTextureDataFrameTransformer::IndexToWorldPos(int32 Index) const {
	const int32 y = Index / Width;
	const int32 x = Index % Width;
	const FVector LocalPos = LocalOffset + LocalScale * FVector(x, y, 0);
	const FVector WorldPos = WorldTransform.TransformPosition(LocalPos);
	return WorldPos;
}


///////////////////////////////////////////////////////


UDungeonForgeTextureSettingsBase::UDungeonForgeTextureSettingsBase() {
	
	DebugMaterial = FSoftObjectPath(TEXT("/DungeonArchitect/Core/Editors/ForgeEditor/Materials/M_VisualizeTexture_Inst.M_VisualizeTexture_Inst"));
}

bool UDungeonForgeTextureSettingsBase::GetTransformToolTransform_Implementation(FTransform& OutTransform) const {
	OutTransform = Transform;
	return true;
}

void UDungeonForgeTextureSettingsBase::SetTransformToolTransform_Implementation(const FTransform& NewTransform, bool bInteractionEnded) {
	Transform = NewTransform;
}

#if WITH_EDITOR
EDungeonForgeSettingsType UDungeonForgeTextureSettingsBase::GetType() const {
	return EDungeonForgeSettingsType::Spatial;
}
#endif // WITH_EDITOR

TArray<FDungeonForgePinProperties> UDungeonForgeTextureSettingsBase::InputPinProperties() const {
	return {};
}

TArray<FDungeonForgePinProperties> UDungeonForgeTextureSettingsBase::OutputPinProperties() const {
	TArray<FDungeonForgePinProperties> PinProperties;
	PinProperties.Emplace(DungeonForgePinConstants::DungeonLayoutTexturesLabel, EDungeonForgeDataType::Texture);
	 
	return PinProperties;
}

FDungeonForgeElementPtr UDungeonForgeTextureSettingsBase::CreateElement() const {
	return MakeShared<FDungeonForgeTextureElementBase>();
}

bool FDungeonForgeTextureElementBase::ExecuteImpl(const FDungeonForgeContextPtr& InContext) const {
	FDungeonForgeTextureContextBasePtr Context = StaticCastSharedPtr<FDungeonForgeTextureContextBase>(InContext);
	
	FDungeonForgeTaggedData TextureTaggedData;
	TextureTaggedData.Pin = DungeonForgePinConstants::DungeonLayoutTexturesLabel;
	UObject* Outer = nullptr;
	TSharedPtr<const FDungeonForgeGraphExecutionState> ExecState = InContext->ExecutionState.Pin();
	if (ExecState.IsValid()) {
		Outer = ExecState->ResourceManager.Get();
	}
	if (!Outer) {
		Outer = GetTransientPackage();
	}
	
	UDungeonForgeTextureData* TextureData = NewObject<UDungeonForgeTextureData>(Outer);
	TextureTaggedData.Data = TextureData;

	UDungeonForgeTextureSettingsBase* NodeSettings = Context->Node.IsValid()
			? Cast<UDungeonForgeTextureSettingsBase>(Context->Node->GetSettings())
			: nullptr;

	if (NodeSettings) {
		TextureData->WorldTransform = NodeSettings->Transform;
		TextureData->BaseScale = NodeSettings->BaseScale;
		TextureData->DebugMaterial = NodeSettings->DebugMaterial;
		TextureData->ColorChannel = NodeSettings->ColorChannel;

		InitializeTextureData(TextureData, NodeSettings);
		
		Context->TextureData = TextureData;
		Context->TargetResolution = NodeSettings->TargetResolution;
	}
	Context->OutputData.AddData(TextureTaggedData);

	if (!DownloadCanvasTextureFromGPU(Context)) {
		// Download failed. Return true so we don't want on the completion of this node
		return true;
	}
	
	if (IsDownloadComplete(Context)) {
		HandleDownloadComplete(Context);
		return true;
	}
	return false;
}

bool FDungeonForgeTextureElementBase::TickImpl(const FDungeonForgeContextPtr& InContext, float DeltaTime) const {
	FDungeonForgeTextureContextBasePtr Context = StaticCastSharedPtr<FDungeonForgeTextureContextBase>(InContext);
	if (!Context->TextureData.IsValid()) {
		// Invalid state. Stop async execution
		return true;
	}
	
	if (IsDownloadComplete(Context)) {
		HandleDownloadComplete(Context);
		return true;
	}
	return false;
}

bool FDungeonForgeTextureElementBase::GetSourceTextureSize(const FDungeonForgeContextPtr& InContext, int32& OutWidth, int32& OutHeight) const {
	return false;
}

bool FDungeonForgeTextureElementBase::InitializeCanvasTexture(UWorld* InWorld, const FDungeonForgeContextPtr& InContext, UCanvasRenderTarget2D* CanvasRT) const {
	return true;
}

FDungeonForgeContextPtr FDungeonForgeTextureElementBase::CreateContext() {
	return MakeShared<FDungeonForgeTextureContextBase>();
}

bool FDungeonForgeTextureElementBase::DownloadCanvasTextureFromGPU(const FDungeonForgeTextureContextBasePtr& Context) const {
	UDungeonForgeTextureSettingsBase* NodeSettings = Context->Node.IsValid()
			? Cast<UDungeonForgeTextureSettingsBase>(Context->Node->GetSettings())
			: nullptr;

	TSharedPtr<const FDungeonForgeGraphExecutionState> ExecState = Context->ExecutionState.Pin();
	if (!ExecState.IsValid()) {
		UE_LOG(LogDungeonForge, Error, TEXT("DungeonForgeStaticTextureElement: Invalid exec state"));
		return false;
	}
	TStrongObjectPtr<UDungeonForgeResourceManager> ResourceManager = ExecState->ResourceManager.Pin();
	if (!ResourceManager.IsValid()) {
		UE_LOG(LogDungeonForge, Error, TEXT("DungeonForgeStaticTextureElement: Invalid resource manager"));
		return false;
	}
	
	UWorld* World = ExecState->Dungeon.IsValid() ? ExecState->Dungeon->GetWorld() : nullptr;
	if (!Context->TextureData.IsValid()) {
		UE_LOG(LogDungeonForge, Error, TEXT("DungeonForgeStaticTextureElement: Invalid texture state"));
		return false;
	}

	UDungeonForgeTextureData* TextureData = Context->TextureData.Get();
	int32 Width{};
	int32 Height{};
	int32 TargetResolution = FMath::Max(1, Context->TargetResolution);
	
	int32 SourceWidth{};
	int32 SourceHeight{};
	if (!GetSourceTextureSize(Context, SourceWidth, SourceHeight)) {
		UE_LOG(LogDungeonForge, Error, TEXT("DungeonForgeStaticTextureElement: Cannot get source texture size"));
		return false;
	}
	
	if (SourceWidth <= 0 || SourceHeight <= 0) {
		UE_LOG(LogDungeonForge, Warning, TEXT("DungeonForgeStaticTextureElement: Source Texture has invalid dimensions. Using defaults"));
		Width = TargetResolution;
		Height = TargetResolution;
	}
	else {
		const float Aspect = static_cast<float>(SourceWidth) / static_cast<float>(SourceHeight);
		if (SourceWidth > SourceHeight) {
			Width = TargetResolution;
			Height = FMath::RoundToInt(static_cast<float>(Width) / Aspect);
		}
		else {
			Height = TargetResolution;
			Width = FMath::RoundToInt(static_cast<float>(Height) * Aspect);
		}

		Width = FMath::Max(1, Width);
		Height = FMath::Max(1, Height);
	}

	Context->DownloadedSize = FIntPoint(Width, Height);
	
	UCanvasRenderTarget2D* CanvasRT = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(World, UCanvasRenderTarget2D::StaticClass(), Width, Height);
	Context->RenderTarget = CanvasRT;
	ResourceManager->RegisterManagedTexture(CanvasRT);

	if (!TextureData->DebugPreviewTexture.IsValid()) {
		TextureData->DebugPreviewTexture = CanvasRT;
	}
	
	if (!InitializeCanvasTexture(World, Context, CanvasRT)) {
		UE_LOG(LogDungeonForge, Warning, TEXT("DungeonForgeStaticTextureElement: Failed to write to canvas render target"));
		Context->RenderTarget = nullptr;
		return false;
	}
	

	TextureData->Width = Width;
	TextureData->Height = Height;

	FTextureRenderTargetResource* RTResource = CanvasRT->GameThread_GetRenderTargetResource();
	if (!RTResource) {
		UE_LOG(LogTemp, Error, TEXT("DungeonForgeStaticTextureElement: Could not get RenderTargetResource from Canvas"));
		Context->RenderTarget = nullptr; // Clear ref if resource invalid
		return false;
	}
	
	int32 NumPixels = CanvasRT->SizeX * CanvasRT->SizeY;
	Context->DownloadedPixels.SetNumUninitialized(NumPixels);
	TextureData->Points.SetNumUninitialized(NumPixels);
	
	Context->RenderFence = {};
	int32 CanvasSizeX = CanvasRT->SizeX;
	int32 CanvasSizeY = CanvasRT->SizeY;
    	
	FDungeonForgeTextureContextBaseWeakPtr WeakContext = Context;
	FIntRect ReadRect(0, 0, CanvasSizeX, CanvasSizeY);
	
	ENQUEUE_RENDER_COMMAND(ReadSurfaceCommandAsync_StaticDownload)([WeakContext, RTResource, NumPixels, ReadRect](FRHICommandListImmediate& RHICmdList) {
		FDungeonForgeTextureContextBasePtr Context = WeakContext.Pin();
		if (Context.IsValid()) {
			RHICmdList.ReadSurfaceData(
				RTResource->GetRenderTargetTexture(),
				ReadRect,
				Context->DownloadedPixels,
				FReadSurfaceDataFlags(RCM_UNorm, CubeFace_MAX)
			);
			
		}
	});

	Context->RenderFence.BeginFence();

	return true;
}

bool FDungeonForgeTextureElementBase::IsDownloadComplete(const FDungeonForgeTextureContextBasePtr& Context) {
	return !Context.IsValid() || Context->RenderFence.IsFenceComplete();
}

void FDungeonForgeTextureElementBase::HandleDownloadComplete(const FDungeonForgeTextureContextBasePtr& Context) {
	if (UDungeonForgeTextureData* TextureData = Context->TextureData.Get()) {
		TArray<FColor>& PixelColors = Context->DownloadedPixels;
		TextureData->Points.SetNumUninitialized(PixelColors.Num());
		FDungeonForgeTextureDataFrameTransformer FrameTransform(TextureData->BaseScale, TextureData->Width, TextureData->Height, TextureData->WorldTransform);
		constexpr float ByteToFloatMUlt = 1.0f / 255.0f; 
		for (int i = 0; i < PixelColors.Num(); i++) {
			float PixelValue = PixelColors[i].R * ByteToFloatMUlt;
			FDungeonForgePoint& TexturePoint = TextureData->Points[i];
			TexturePoint.Density = PixelValue;
			TexturePoint.Transform = FTransform(FrameTransform.IndexToWorldPos(i));
		}
	}
}


#undef LOCTEXT_NAMESPACE

