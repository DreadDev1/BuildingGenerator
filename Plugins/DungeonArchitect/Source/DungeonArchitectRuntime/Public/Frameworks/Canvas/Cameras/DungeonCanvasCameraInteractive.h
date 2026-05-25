//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Canvas/Cameras/DungeonCanvasCamera.h"
#include "DungeonCanvasCameraInteractive.generated.h"

UCLASS(Blueprintable)
class DUNGEONARCHITECTRUNTIME_API UDungeonCanvasCameraInteractive : public UDungeonCanvasCamera {
	GENERATED_BODY()
public:
	virtual void Initialize_Implementation(UDungeonCanvasComponent* DungeonCanvas) override;
	virtual void Update_Implementation(UDungeonCanvasComponent* DungeonCanvas, float DeltaSeconds) override;
	virtual bool OnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual bool OnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual bool OnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual bool OnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& InMouseEvent) override;

public:
	/** Zoom speed - how much the log zoom changes per wheel tick */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Canvas")
	float ZoomSpeed = 1.0f;

	/** Pan speed multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Canvas")
	float PanSpeed = 1.0f;

	/** Maximum zoom in level (0.1 = 10x zoom in from initial view) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Canvas")
	float MaxZoomIn = 0.1f;

	/** How fast the zoom interpolates to target (higher = faster, 0 = instant) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Canvas", meta=(ClampMin="0.0"))
	float ZoomInterpSpeed = 10.0f;

private:
	bool bIsPanning = false;
	FVector2D LastMousePosition;

	/** Base world size captured at initialization (represents the full dungeon view) */
	FVector BaseWorldSize = FVector::OneVector;

	/** Current zoom level (1.0 = full dungeon view, <1 = zoomed in) */
	float CurrentZoom = 1.0f;

	/** Target zoom level for smooth interpolation */
	float TargetZoom = 1.0f;
};

