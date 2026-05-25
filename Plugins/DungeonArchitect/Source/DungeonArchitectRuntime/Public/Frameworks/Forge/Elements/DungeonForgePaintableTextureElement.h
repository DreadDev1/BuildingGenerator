//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/DungeonForgeElement.h"
#include "Frameworks/Forge/Elements/DungeonForgeTextureElementBase.h"
#include "Frameworks/Forge/Elements/Interfaces/DungeonForgePaintBrushToolHandler.h"
#include "DungeonForgePaintableTextureElement.generated.h"

class UCanvasRenderTarget2D;

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonForgePaintableTextureSettings
	: public UDungeonForgeTextureSettingsBase
	, public IDungeonForgePaintBrushToolHandler
{
	GENERATED_BODY()
	
public:	
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override;
	virtual FText GetDefaultNodeTitle() const override;
	virtual FText GetNodeTooltipText() const override;
#endif // WITH_EDITOR

	
	virtual bool GetPaintCanvasTransform_Implementation(FTransform& OutTransform) const override;
	virtual void ApplyPaintStroke_Implementation(const FVector& WorldHitLocation, const FVector& LocalHitLocation) override;
	
protected:
	virtual FDungeonForgeElementPtr CreateElement() const override;
};

class DUNGEONARCHITECTRUNTIME_API FDungeonForgePaintableTextureElement : public FDungeonForgeTextureElementBase {
protected:
	virtual bool GetSourceTextureSize(const FDungeonForgeContextPtr& InContext, int32& OutWidth, int32& OutHeight) const override;
	virtual bool InitializeCanvasTexture(UWorld* InWorld, const FDungeonForgeContextPtr& InContext, UCanvasRenderTarget2D* CanvasRT) const override;
};

