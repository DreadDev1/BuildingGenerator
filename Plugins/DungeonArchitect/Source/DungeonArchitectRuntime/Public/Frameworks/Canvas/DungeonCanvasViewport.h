//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DungeonCanvasViewport.generated.h"

struct FDungeonLayoutData;

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonCanvasViewportTransform {
	GENERATED_BODY()
public:
	FDungeonCanvasViewportTransform() {}
	void SetLocalToWorld(const FTransform& InTransform);
	void SetLocalToCanvas(const FTransform& InTransform);
	void FocusOnCanvas(float Width, float Height);
	FTransform GetLocalToWorld() const { return LocalToWorld; }
	FTransform GetLocalToCanvas() const { return LocalToCanvas; }
	
	FTransform GetWorldToCanvas() const { return WorldToCanvas; }
	FTransform GetCanvasToWorld() const { return CanvasToWorld; }

	FVector2D WorldToCanvasLocation(const FVector& InWorldLocation) const;
	FVector CanvasToWorldLocation(const FVector2D& InCanvasLocation) const;

	FVector2D CanvasToUV(const FVector2D& InCanvasLocation) const;
	FVector2D UVToCanvas(const FVector2D& InUV) const;
	
	bool IsFocusInitialized() const { return bFocusInitialized; }

private:
	void RecalculateTransforms();
	void UpdateWorldAspectRatio();
	
private:
	
	UPROPERTY()
	FTransform LocalToWorld = FTransform::Identity;
	
	UPROPERTY()
	FTransform LocalToCanvas = FTransform::Identity;
	
	UPROPERTY()
	FTransform WorldToCanvas = FTransform::Identity;
	
	UPROPERTY()
	FTransform CanvasToWorld = FTransform::Identity;
	
private:
	UPROPERTY()
	bool bFocusInitialized = false;
};

