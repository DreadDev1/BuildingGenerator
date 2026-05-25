//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/DungeonForgeElement.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"
#include "DungeonForgeCreateLayoutTextures.generated.h"

namespace DungeonForgeCreateLayoutTexturesPins {
	const FName TextureSDF = TEXT("TexSDF");
	const FName TextureHeights = TEXT("TexHeights");
}


UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeCreateLayoutTexturesSettings : public UDungeonForgeNodeSettings {
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Settings")
	int32 LayoutTextureSize = 1024;
	
	UPROPERTY(EditAnywhere, Category = "Settings")
	float PaddingWorldUnits = 1000;
	
	UPROPERTY(EditAnywhere, Category = "Settings")
	int32 FloorIndex = 0;
	
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bDrawAllFloors = true;
	
	UPROPERTY(EditAnywhere, Category = "Debug")
	TSoftObjectPtr<UMaterialInterface> DebugMaterial;
	
public:
	UDungeonForgeCreateLayoutTexturesSettings();
	
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override;
	virtual FText GetDefaultNodeTitle() const override;
	virtual FText GetNodeTooltipText() const override;
	virtual EDungeonForgeSettingsType GetType() const override;
#endif // WITH_EDITOR

	
protected:
	virtual TArray<FDungeonForgePinProperties> InputPinProperties() const override;
	virtual TArray<FDungeonForgePinProperties> OutputPinProperties() const override;
	virtual FDungeonForgeElementPtr CreateElement() const override;
};

struct FDungeonForgeLayoutTextureContext : FDungeonForgeContext {
	FRenderCommandFence TextureRenderFence{};
};
typedef TSharedPtr<FDungeonForgeLayoutTextureContext, ESPMode::ThreadSafe> FDungeonForgeLayoutTextureContextPtr;

class FDungeonForgeCreateLayoutTexturesElement : public IDungeonForgeElement {
protected:
	virtual FDungeonForgeContextPtr CreateContext() override;
	virtual bool ExecuteImpl(const FDungeonForgeContextPtr& Context) const override;
	virtual bool TickImpl(const FDungeonForgeContextPtr& Context, float DeltaTime) const override;
};

