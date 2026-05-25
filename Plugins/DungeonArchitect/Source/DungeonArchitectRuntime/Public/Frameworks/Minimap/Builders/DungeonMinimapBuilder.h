//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

class UDungeonCanvasTrackedObject;
struct FDungeonCanvasOverlayIcon;
class UDungeonModel;
struct FDungeonLayoutData;
class UTexture;
class UTexture2D;
class UTextureRenderTarget2D;

class FDungeonMinimapBuilder {
public:
	virtual ~FDungeonMinimapBuilder() = default;

	struct FLayoutBuildSettings {
		int32 TextureSize{};
		float OutlineThickness{};
		float DoorThickness{};
		float BlurRadius{};
		int32 BlurIterations{};
	};
	
	virtual void BuildLayoutTexture(UWorld* InWorld, const FTransform& WorldToScreen, const FLayoutBuildSettings& InSettings, const FDungeonLayoutData& LayoutData, const UDungeonModel* InDungeonModel, TObjectPtr<UTexture>& OutMaskTexture);
	virtual void BuildStaticOverlayTexture(UWorld* InWorld, const FTransform& WorldToScreen, UTextureRenderTarget2D* InTexture, const FDungeonLayoutData& LayoutData,
			const TArray<FDungeonCanvasOverlayIcon>& OverlayIcons) const;
	virtual void UpdateDynamicOverlayTexture(UWorld* InWorld, const FTransform& WorldToScreen, UTextureRenderTarget2D* DynamicOverlayTexture,
		const TArray<UDungeonCanvasTrackedObject*>& DynamicTracking, TArray<FDungeonCanvasOverlayIcon> OverlayIcons);

	struct FFogOfWarUpdateSettings {
		FTransform WorldToScreen{};
		FName FogOfWarTrackingItem{};
		UTexture2D* FogOfWarExploreTexture{};
		float FogOfWarVisibilityDistance{};
	};
	virtual void UpdateFogOfWarTexture(UWorld* InWorld, UTextureRenderTarget2D* InFogOfWarTexture, const TArray<UDungeonCanvasTrackedObject*>& InTrackedObjects, const FFogOfWarUpdateSettings& InSettings) const;
};
typedef TSharedPtr<FDungeonMinimapBuilder> FDungeonMinimapBuilderPtr;

