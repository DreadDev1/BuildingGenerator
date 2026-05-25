//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/DungeonForgeContext.h"
#include "Frameworks/Forge/DungeonForgeElement.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"
#include "DungeonForgeApplyTheme.generated.h"

class UDungeonThemeAsset;
class FDungeonSceneProvider;
class UDungeonForgeMarkerData;
class FDungeonSceneCollector;

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeApplyThemeSettings : public UDungeonForgeNodeSettings {
	GENERATED_BODY()
public:	
	UPROPERTY(EditAnywhere, Category = "Settings")
	TObjectPtr<UDungeonThemeAsset> ThemeAsset;

	/** Should we override the dungeon config's instance setting with the instanced setting below */ 
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bOverrideInstancedSetting = false;

	/** Only valid if bOverrideInstancedSetting is set to true, this setting overrides the dungeon config's instance flag */ 
	UPROPERTY(EditAnywhere, Category = "Settings", meta=(EditCondition = "bOverrideInstancedSetting"))
	bool bInstanced = false;

	/** Should we override the dungeon config's frame build time setting with the setting below? */ 
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bOverrideFrameBuildTimeMs = false;

	/** Only valid if bOverrideFrameBuildTimeMs is set to true, this setting overrides the dungeon config's frame build time setting */ 
	UPROPERTY(EditAnywhere, Category = "Settings", meta=(EditCondition = "bOverrideFrameBuildTimeMs"))
	int32 FrameBuildTimeMs = 0;
	
public:
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

struct FDungeonForgeApplyThemeContext : public FDungeonForgeContext {
	struct FSceneBuilder {
		TSharedPtr<FDungeonSceneProvider> SceneProvider;
		float FrameTimeMs = 0;
	};
	TArray<FSceneBuilder> SceneBuilders;
};
typedef TSharedPtr<FDungeonForgeApplyThemeContext, ESPMode::ThreadSafe> FDungeonForgeApplyThemeContextPtr;

class FDungeonForgeApplyThemeElement : public IDungeonForgeElement {
protected:
	virtual bool ExecuteImpl(const FDungeonForgeContextPtr& Context) const override;
	virtual bool TickImpl(const FDungeonForgeContextPtr& Context, float DeltaTime) const override;
	virtual FDungeonForgeContextPtr CreateContext() override;

private:
	void ApplyTheme(const FDungeonForgeApplyThemeContextPtr& InContext, const TObjectPtr<UDungeonThemeAsset>& InThemeAsset, const UDungeonForgeMarkerData* InMarkerData) const;
	
	static TSharedPtr<FDungeonSceneProvider> CreateSceneProvider(const UDungeonForgeMarkerData* InMarkerData, const UDungeonForgeApplyThemeSettings* InNodeSettings);
	static bool IsInstanced(const UDungeonForgeMarkerData* InMarkerData, const UDungeonForgeApplyThemeSettings* NodeSettings);
	static float GetFrameBuildTimeMs(const UDungeonForgeMarkerData* InMarkerData, const UDungeonForgeApplyThemeSettings* NodeSettings);
	static bool IsRunning(const FDungeonForgeApplyThemeContextPtr& InContext);
	
};

