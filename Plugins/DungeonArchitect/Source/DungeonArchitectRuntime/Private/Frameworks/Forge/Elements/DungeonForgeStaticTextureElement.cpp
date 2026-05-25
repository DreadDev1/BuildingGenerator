//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Elements/DungeonForgeStaticTextureElement.h"

#include "Core/Utils/DungeonLog.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"

#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"

#define LOCTEXT_NAMESPACE "DungeonForgeStaticTextureSettings"

#if WITH_EDITOR
FName UDungeonForgeStaticTextureSettings::GetDefaultNodeName() const {
	return TEXT("StaticTexture");
}

FText UDungeonForgeStaticTextureSettings::GetDefaultNodeTitle() const {
	return LOCTEXT("NodeTitle", "Static Texture");
}

FText UDungeonForgeStaticTextureSettings::GetNodeTooltipText() const {
	return LOCTEXT("NodeTooltip", "Import a static texture");
}

#endif // WITH_EDITOR

FDungeonForgeElementPtr UDungeonForgeStaticTextureSettings::CreateElement() const {
	return MakeShared<FDungeonForgeStaticTextureElement>();
}


bool FDungeonForgeStaticTextureElement::GetSourceTextureSize(const FDungeonForgeContextPtr& InContext, int32& OutWidth, int32& OutHeight) const {
	UDungeonForgeStaticTextureSettings* NodeSettings = InContext.IsValid() && InContext->Node.IsValid()
			? Cast<UDungeonForgeStaticTextureSettings>(InContext->Node->GetSettings())
			: nullptr;

	if (!NodeSettings || !NodeSettings->Texture) {
		return false;
	}
	
	OutWidth = NodeSettings->Texture->GetSurfaceWidth();
	OutHeight = NodeSettings->Texture->GetSurfaceHeight();
	return true;
}

bool FDungeonForgeStaticTextureElement::InitializeCanvasTexture(UWorld* InWorld, const FDungeonForgeContextPtr& InContext, UCanvasRenderTarget2D* CanvasRT) const {
	FDungeonForgeTextureContextBasePtr Context = StaticCastSharedPtr<FDungeonForgeTextureContextBase>(InContext);
	UDungeonForgeStaticTextureSettings* NodeSettings = Context->Node.IsValid()
			? Cast<UDungeonForgeStaticTextureSettings>(Context->Node->GetSettings())
			: nullptr;

	UTexture* Texture = NodeSettings ? NodeSettings->Texture : nullptr;
	if (!Texture) {
		UE_LOG(LogDungeonForge, Warning, TEXT("DungeonForgeStaticTextureElement: Cannot initialize canvas texture"));
		return false;
	}

	UCanvas* Canvas = nullptr;
	FVector2D CanvasSize;
	FDrawToRenderTargetContext DrawContext;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(InWorld, CanvasRT, Canvas, CanvasSize, DrawContext);
	if (!Canvas) {
		UE_LOG(LogDungeonForge, Warning, TEXT("DungeonForgeStaticTextureElement: Failed to write to canvas render target"));
		Context->RenderTarget = nullptr;
		return false;
	}
	
	Canvas->K2_DrawTexture(NodeSettings->Texture, FVector2D::ZeroVector, CanvasSize,
		FVector2D::ZeroVector, FVector2D::UnitVector,
		FLinearColor::White, BLEND_Opaque);
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(InWorld, DrawContext);
	return true;
}

void FDungeonForgeStaticTextureElement::InitializeTextureData(UDungeonForgeTextureData* InTextureData, UDungeonForgeTextureSettingsBase* InNodeSettings) const {
	FDungeonForgeTextureElementBase::InitializeTextureData(InTextureData, InNodeSettings);
	if (UDungeonForgeStaticTextureSettings* TexNodeSettings = Cast<UDungeonForgeStaticTextureSettings>(InNodeSettings)) {
		InTextureData->DebugPreviewTexture = TexNodeSettings->Texture;
	}
}

#undef LOCTEXT_NAMESPACE

