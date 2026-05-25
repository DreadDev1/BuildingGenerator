//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/PCG/Data/PCGSGFTextureData.h"
#include "Frameworks/PCG/DungeonPCGCommon.h"

#include "Elements/PCGTextureSampler.h"
#include "PCGForgeNodeGetLayoutTexture.generated.h"

class UTexture;
class UHeatmapColorRamp;

UCLASS(BlueprintType, ClassGroup = (Procedural))
class DUNGEONARCHITECTRUNTIME_API UPCGForgeLayoutTextureSamplerSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	UPCGForgeLayoutTextureSamplerSettings(const FObjectInitializer& ObjectInitializer);
	
	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("DungeonForgeLayoutTextures")); }
	virtual FText GetDefaultNodeTitle() const override;
	virtual FText GetNodeTooltipText() const override;
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Spatial; }
	virtual bool CanDynamicallyTrackKeys() const override { return true; }
	//virtual bool CanDynamicallyTrackKeys() const override { return true; }
	
#endif

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override { return TArray<FPCGPinProperties>(); }
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	EPCGTextureColorChannel ColorChannel = EPCGTextureColorChannel::Alpha;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	EPCGTextureFilter Filter = EPCGTextureFilter::Bilinear;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (UIMin = "1.0", ClampMin = "1.0", PCG_Overridable))
	float SampleGridSize = 200.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings)
	TSoftObjectPtr<UHeatmapColorRamp> HeatmapColorRamp;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	bool bUseAdvancedTiling = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tiling", meta = (EditCondition = "bUseAdvancedTiling", PCG_Overridable))
	FVector2D Tiling = FVector2D(1.0, 1.0);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tiling", meta = (EditCondition = "bUseAdvancedTiling", PCG_Overridable))
	FVector2D CenterOffset = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tiling", meta = (UIMin = -360, ClampMin = -360, UIMax = 360, ClampMax = 360, Units = deg, EditCondition = "bUseAdvancedTiling", PCG_Overridable))
	float Rotation = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tiling", meta = (EditionCondition = "bUseAdvancedTiling", PCG_Overridable))
	bool bUseTileBounds = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tiling", meta = (EditCondition = "bUseAdvancedTiling && bUseTileBounds", PCG_Overridable))
	FVector2D TileBoundsMin = FVector2D(-0.5, -0.5);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tiling", meta = (EditCondition = "bUseAdvancedTiling && bUseTileBounds", PCG_Overridable))
	FVector2D TileBoundsMax = FVector2D(0.5, 0.5);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Debug")
	bool bSynchronousLoad = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Settings)
	int32 FloorIndex = 0;
	
protected:
	friend class FPCGDungeonSDFTextureSamplerElement;
};


struct FPCGForgeLayoutTextureSamplerContext : public FPCGContext, public IPCGAsyncLoadingContext
{
	virtual ~FPCGForgeLayoutTextureSamplerContext() override;
	EDungeonPCGDataLoadState SDFPointDataLoadState = EDungeonPCGDataLoadState::NotLoaded;
	TObjectPtr<UPCGDungeonSGFTextureData> SDFPointData = nullptr;
};

class FPCGForgeLayoutTextureSamplerElement : public IPCGElement
{
public:
	virtual void GetDependenciesCrc(const FPCGGetDependenciesCrcParams& InParams, FPCGCrc& OutCrc) const override;
	// Loading needs to be done on the main thread and accessing objects outside of PCG might not be thread safe, so taking the safe approach
	virtual bool CanExecuteOnlyOnMainThread(FPCGContext* Context) const override { return true; }
	virtual bool IsCacheable(const UPCGSettings* InSettings) const override { return false; }

protected:
	virtual FPCGContext* CreateContext() override;
	virtual bool ExecuteInternal(FPCGContext* InContext) const override;
	virtual bool PrepareDataInternal(FPCGContext* Context) const override;

};

