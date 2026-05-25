//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/Data/DungeonForgeTextureData.h"
#include "Frameworks/Forge/DungeonForgeElement.h"
#include "Frameworks/Forge/Elements/DungeonForgeTextureElementBase.h"
#include "DungeonForgeStaticTextureElement.generated.h"

class UCanvasRenderTarget2D;

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeStaticTextureSettings
	: public UDungeonForgeTextureSettingsBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "Settings")
	TObjectPtr<UTexture> Texture;
	
public:
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override;
	virtual FText GetDefaultNodeTitle() const override;
	virtual FText GetNodeTooltipText() const override;
#endif // WITH_EDITOR
	
protected:
	virtual FDungeonForgeElementPtr CreateElement() const override;
};

class DUNGEONARCHITECTRUNTIME_API FDungeonForgeStaticTextureElement : public FDungeonForgeTextureElementBase {
protected:
	virtual bool GetSourceTextureSize(const FDungeonForgeContextPtr& InContext, int32& OutWidth, int32& OutHeight) const override;
	virtual bool InitializeCanvasTexture(UWorld* InWorld, const FDungeonForgeContextPtr& InContext, UCanvasRenderTarget2D* CanvasRT) const override;
	virtual void InitializeTextureData(UDungeonForgeTextureData* InTextureData, UDungeonForgeTextureSettingsBase* InNodeSettings) const override;
};


