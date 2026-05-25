//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Canvas/DungeonCanvasViewport.h"

#include "Components/SceneComponent.h"
#include "DungeonCanvasStructs.generated.h"

//////////////////////// Object Tracking ////////////////////////

class UDungeonCanvasMaterialTheme;
class UDungeonCanvasBlueprint;
class ADungeon;
class UCanvas;
class UCanvasRenderTarget2D;
class UDungeonCanvasEffectBase;
class UDungeonModel;
class UDungeonConfig;
class UDungeonEditorViewportProperties;
class USceneComponent;
class UDungeonCanvasCamera;
class UDungeonCanvasActorIconFilter;
class UDungeonCanvasThemeRenderResources;
class UDungeonCanvasTrackedObject;

struct FDungeonFloorSettings;
struct FDungeonCanvasViewportTransform;

UENUM(BlueprintType)
enum class EDungeonCanvasIconCoordinateSystem : uint8 {
	Pixels = 0 UMETA(DisplayName = "Pixels"),
	WorldCoordinates UMETA(DisplayName = "World Coordinates"),
};

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonCanvasOverlayInternalIcon {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas", meta=(AllowPrivateAccess="true", DisplayThumbnail="true", DisplayName="Image", AllowedClasses="/Script/Engine.Texture,/Script/Engine.MaterialInterface,/Script/Engine.SlateTextureAtlasInterface", DisallowedClasses = "/Script/MediaAssets.MediaTexture"))
	TSoftObjectPtr<UObject> ResourceObject;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	FLinearColor Tint = FLinearColor::White;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	float RotationOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	float Scale = 1.0f;
	
	/** The base rotation will be ignored.   Enable this if you want the icon to always face a certain angle.  E.g. When the whole map rotates when the player looks around,  if you have an up or down arrow in a lift room, you don't this arrow icon to rotate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	bool bAbsoluteRotation = false;
};

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonCanvasOverlayIcon {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas", meta=(AllowPrivateAccess="true", DisplayThumbnail="true", DisplayName="Image", AllowedClasses="/Script/Engine.Texture,/Script/Engine.MaterialInterface,/Script/Engine.SlateTextureAtlasInterface", DisallowedClasses = "/Script/MediaAssets.MediaTexture"))
	TObjectPtr<UObject> ResourceObject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	float ScreenSize = 32.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	EDungeonCanvasIconCoordinateSystem ScreenSizeType = EDungeonCanvasIconCoordinateSystem::Pixels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	float AspectRatio = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	FLinearColor Tint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	float RotationOffset = 0.0f;

	/** The base rotation will be ignored.   Enable this if you want the icon to always face a certain angle.  E.g. When the whole map rotates when the player looks around,  if you have an up or down arrow in a lift room, you don't this arrow icon to rotate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	bool bAbsoluteRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	TArray<FName> Tags;
};


USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonCanvasDrawSettings {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Architect")
	bool bFogOfWarEnabled = false;

	/** start with fog of war map fully explored */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Architect")
	bool bFogOfWarFullyExplored = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Architect")
	FLinearColor BackgroundColor = FLinearColor::Transparent;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Architect")
	bool TextureBorderEnabled = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Architect")
	FLinearColor TextureBorderColor = FLinearColor::Black;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Architect")
	bool bDrawTransparentBackgroundCheckerboardTexture = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, SimpleDisplay, Category = Advanced)
	TArray<TObjectPtr<UDungeonCanvasActorIconFilter>> OverlayActorIconFilters;
};

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonCanvasDrawContext {
	GENERATED_BODY()
	
	/** The main dungeon layout and its material background is drawn in this layout */
	UPROPERTY(BlueprintReadOnly, Category="DungeonCanvas")
	TObjectPtr<UCanvas> CanvasLayout = {};;

	UPROPERTY(BlueprintReadOnly, Category="DungeonCanvas")
	FDungeonCanvasDrawSettings DrawSettings;
	
	UPROPERTY(BlueprintReadOnly, Category="DungeonCanvas")
	TObjectPtr<UDungeonCanvasMaterialTheme> Theme = {};;
	
	UPROPERTY(BlueprintReadOnly, Category="DungeonCanvas")
	TObjectPtr<UDungeonCanvasThemeRenderResources> ThemeRenderResources = {};;

	UPROPERTY(BlueprintReadOnly, Category="DungeonCanvas")
	int32 FloorIndex = 0;
	
	UPROPERTY(BlueprintReadOnly, Category="DungeonCanvas")
	FDungeonCanvasViewportTransform ViewTransform;
};

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonCanvasTrackedActorRegistryItem {
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category="Dungeon Canvas")
	TWeakObjectPtr<UDungeonCanvasTrackedObject> TrackedComponent;

	UPROPERTY()
	FGuid InstanceID;
};

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonCanvasItemFogOfWarSettings {
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Canvas")
	float LightRadius = 2000;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Canvas")
	int NumShadowSamples = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Canvas")
	float ShadowJitterDistance = 30;
};

