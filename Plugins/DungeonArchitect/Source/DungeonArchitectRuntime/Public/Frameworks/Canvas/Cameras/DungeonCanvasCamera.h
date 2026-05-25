//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Canvas/DungeonCanvasViewport.h"

#include "Input/Events.h"
#include "Layout/Geometry.h"
#include "DungeonCanvasCamera.generated.h"

class UDungeonCanvasComponent;


UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class DUNGEONARCHITECTRUNTIME_API UDungeonCanvasCamera : public UObject {
	GENERATED_BODY()

public:
	UPROPERTY()
	FDungeonCanvasViewportTransform ViewportTransform;
	
	UPROPERTY()
	FRotator CameraRotation = FRotator::ZeroRotator;

	/**
	 * Should the world rotate around the fixed player icon?
	 * True: Player icon is fixed and world rotates around it
	 * False: World rotation is fixed, and player icon rotates
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Canvas")
	bool bRotateToView = false;

	/** The base canvas rotation, in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Canvas")
	float BaseCanvasRotation = 0;

	/**
	 * The active floor index, may be ignored, if the canvas is configured to render every floor
	 * (i.e. doesn't have overlapping floor geometry, like the grid builder)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Canvas")
	int32 FloorIndex = 0;
	
	/** The owning player controller, may be null */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Canvas")
	TObjectPtr<APlayerController> PlayerController;
	
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera")
	void Initialize(UDungeonCanvasComponent* DungeonCanvas);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera")
	void Update(UDungeonCanvasComponent* DungeonCanvas, float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "Dungeon Canvas")
	void SetCameraLocation(FVector WorldLocation);
	
	UFUNCTION(BlueprintCallable, Category = "Dungeon Canvas")
	void SetCameraSize(FVector WorldSize);
	
	UFUNCTION(BlueprintCallable, Category = "Dungeon Canvas")
	void SetCameraRotation(FRotator Rotation);
	
	UFUNCTION(BlueprintCallable, Category = "Dungeon Canvas")
	void SetCameraWorldTransform(FTransform WorldTransform);

	UFUNCTION(BlueprintCallable, Category = "Dungeon Canvas")
	void SetCanvasTransform(const FDungeonCanvasViewportTransform& Transform);
	
	virtual bool OnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent);
	virtual bool OnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent);
	virtual bool OnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent);
	virtual bool OnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent);
	virtual void OnMouseLeave(const FPointerEvent& InMouseEvent);
public:
	FDungeonCanvasViewportTransform GetCameraTransform(const FVector2D& InCanvasSize) const;
};

