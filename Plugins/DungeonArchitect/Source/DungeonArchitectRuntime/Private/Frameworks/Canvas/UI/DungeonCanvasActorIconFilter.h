//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DungeonCanvasActorIconFilter.generated.h"

struct FDungeonCanvasViewportTransform;
struct FDungeonCanvasOverlayIcon;
class UCurveFloat;

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable, Abstract, HideDropdown)
class DUNGEONARCHITECTRUNTIME_API UDungeonCanvasActorIconFilter : public UObject {
	GENERATED_BODY()
public:
	virtual void ApplyFilter(const FDungeonCanvasViewportTransform& View, const TMap<FName, FDungeonCanvasOverlayIcon>& OverlayIconMap,
		const TArray<FName>& ItemTags, TObjectPtr<UObject>& ResourceObject, FVector2D& CanvasSize, FVector2D& CanvasLocation,
		float& ScreenSize, FLinearColor& Tint, float& Rotation) { }
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonCanvasCircularFrameActorIconFilter : public UDungeonCanvasActorIconFilter {
	GENERATED_BODY()
public:
	virtual void ApplyFilter(const FDungeonCanvasViewportTransform& View, const TMap<FName, FDungeonCanvasOverlayIcon>& OverlayIconMap,
		const TArray<FName>& ItemTags, TObjectPtr<UObject>& ResourceObject, FVector2D& CanvasSize, FVector2D& CanvasLocation,
		float& ScreenSize, FLinearColor& Tint, float& Rotation) override;

public:
	UPROPERTY(EditAnywhere, Category="Dungeon Canvas", meta=(UIMin=0, UIMax=1))
	float BoundaryRadius = 1;

	/** The icon to show when the object is out of the circular bounds, e.g. an arrow of sorts */
	UPROPERTY(EditAnywhere, Category="Dungeon Canvas")
	FName BoundaryEdgeIconName;

	/** How far should the object be from the edge before we remove the edge icon (we don't want to show arrows for far away objects) */
	UPROPERTY(EditAnywhere, Category="Dungeon Canvas")
	float BoundaryEdgeIconMaxDisplayDistance = 0.15f;
	
	UPROPERTY(EditAnywhere, Category="Dungeon Canvas")
	TArray<FName> TrackedIconTags;
	
	UPROPERTY(EditAnywhere, Category="Dungeon Canvas")
	bool bEnableScaleBeyondBoundary = true;

	UPROPERTY(EditAnywhere, Category="Dungeon Canvas", meta=(EditCondition="bEnableScaleBeyondBoundary"))
	TSoftObjectPtr<UCurveFloat> ScaleCurve;
};


