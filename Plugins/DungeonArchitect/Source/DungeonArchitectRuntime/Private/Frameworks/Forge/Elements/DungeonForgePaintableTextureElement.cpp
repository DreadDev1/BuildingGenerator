//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Elements/DungeonForgePaintableTextureElement.h"

#include "Core/Utils/DungeonLog.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"

#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"

#define LOCTEXT_NAMESPACE "DungeonForgeStaticTextureSettings"

#if WITH_EDITOR
FName UDungeonForgePaintableTextureSettings::GetDefaultNodeName() const {
	return TEXT("PaintableTexture");
}

FText UDungeonForgePaintableTextureSettings::GetDefaultNodeTitle() const {
	return LOCTEXT("NodeTitle", "Paintable Texture");
}

FText UDungeonForgePaintableTextureSettings::GetNodeTooltipText() const {
	return LOCTEXT("NodeTooltip", "Create a paintable texture");
}
#endif // WITH_EDITOR

bool UDungeonForgePaintableTextureSettings::GetPaintCanvasTransform_Implementation(FTransform& OutTransform) const {
	OutTransform = Transform;
	return true;
}

void UDungeonForgePaintableTextureSettings::ApplyPaintStroke_Implementation(const FVector& WorldHitLocation, const FVector& LocalHitLocation) {
	// TODO: 
}

FDungeonForgeElementPtr UDungeonForgePaintableTextureSettings::CreateElement() const {
	return MakeShared<FDungeonForgePaintableTextureElement>();
}


bool FDungeonForgePaintableTextureElement::GetSourceTextureSize(const FDungeonForgeContextPtr& InContext, int32& OutWidth, int32& OutHeight) const {
	UDungeonForgePaintableTextureSettings* NodeSettings = InContext.IsValid() && InContext->Node.IsValid()
			? Cast<UDungeonForgePaintableTextureSettings>(InContext->Node->GetSettings())
			: nullptr;

	FDungeonForgeTextureContextBasePtr Context = StaticCastSharedPtr<FDungeonForgeTextureContextBase>(InContext);
	OutWidth = NodeSettings->TargetResolution;
	OutHeight = NodeSettings->TargetResolution;
	return true;
}

bool FDungeonForgePaintableTextureElement::InitializeCanvasTexture(UWorld* InWorld, const FDungeonForgeContextPtr& InContext, UCanvasRenderTarget2D* CanvasRT) const {
	FDungeonForgeTextureContextBasePtr Context = StaticCastSharedPtr<FDungeonForgeTextureContextBase>(InContext);
	UDungeonForgePaintableTextureSettings* NodeSettings = Context->Node.IsValid()
			? Cast<UDungeonForgePaintableTextureSettings>(Context->Node->GetSettings())
			: nullptr;

	UCanvas* Canvas = nullptr;
	FVector2D CanvasSize;
	FDrawToRenderTargetContext DrawContext;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(InWorld, CanvasRT, Canvas, CanvasSize, DrawContext);
	if (!Canvas) {
		UE_LOG(LogDungeonForge, Warning, TEXT("DungeonForgeStaticTextureElement: Failed to write to canvas render target"));
		Context->RenderTarget = nullptr;
		return false;
	}

	Canvas->K2_DrawBox(FVector2D(10, 10), FVector2D(20, 20), 5);
	
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(InWorld, DrawContext);
	return true;
}


#undef LOCTEXT_NAMESPACE



