//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DungeonForgePaintBrushToolHandler.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UDungeonForgePaintBrushToolHandler : public UInterface
{
	GENERATED_BODY()
};

class DUNGEONARCHITECTRUNTIME_API IDungeonForgePaintBrushToolHandler
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Dungeon Forge|Paint Brush Tool")
	bool GetPaintCanvasTransform(FTransform& OutTransform) const;
	virtual bool GetPaintCanvasTransform_Implementation(FTransform& OutTransform) const = 0;

	UFUNCTION(BlueprintNativeEvent, Category = "Dungeon Forge|Paint Brush Tool")
	void StartPainting();
	virtual void StartPainting_Implementation() {};

	UFUNCTION(BlueprintNativeEvent, Category = "Dungeon Forge|Paint Brush Tool")
	void ApplyPaintStroke(const FVector& WorldHitLocation, const FVector& LocalHitLocation);
	virtual void ApplyPaintStroke_Implementation(const FVector& WorldHitLocation, const FVector& LocalHitLocation) = 0;

	UFUNCTION(BlueprintNativeEvent, Category = "Dungeon Forge|Paint Brush Tool")
	void EndPainting();
	virtual void EndPainting_Implementation() {};

	UFUNCTION(BlueprintNativeEvent, Category = "Dungeon Forge|Paint Brush Tool")
	void OnPaintToolActivated();
	virtual void OnPaintToolActivated_Implementation() {};

	UFUNCTION(BlueprintNativeEvent, Category = "Dungeon Forge|Paint Brush Tool")
	void OnPaintToolDeactivated();
	virtual void OnPaintToolDeactivated_Implementation() {};
};

