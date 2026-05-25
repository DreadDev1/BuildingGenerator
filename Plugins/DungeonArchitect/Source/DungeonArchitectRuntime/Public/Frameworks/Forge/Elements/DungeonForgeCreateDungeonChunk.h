//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskCreateCellsGrid.h"
#include "Frameworks/Forge/DungeonForgeContext.h"
#include "Frameworks/Forge/DungeonForgeElement.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"
#include "Frameworks/Forge/Elements/Interfaces/DungeonForgeRandomizeToolHandler.h"
#include "Frameworks/Forge/Elements/Interfaces/DungeonForgeTransformToolHandler.h"
#include "DungeonForgeCreateDungeonChunk.generated.h"

class ADungeon;
class UDungeonConfig;
class UDungeonBuilder;

namespace DungeonForgeCreateGridChunkPins {
	const FName Location = TEXT("Location");
}


UCLASS(Abstract)
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeCreateDungeonChunkBaseSettings
	: public UDungeonForgeNodeSettings
	, public IDungeonForgeTransformToolHandler
	, public IDungeonForgeRandomizeToolHandler
{
	GENERATED_BODY()
public:
	
	UPROPERTY()
	TObjectPtr<UDungeonConfig> DungeonConfig{};

	UPROPERTY(EditAnywhere, Category = "Settings")
	FTransform BuildTransform = FTransform::Identity;

protected:
	UPROPERTY()
	TSubclassOf<UDungeonBuilder> BuilderClass = nullptr;
	
public:
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override;
	virtual FText GetDefaultNodeTitle() const override;

	virtual FText GetNodeTooltipText() const override;
	virtual EDungeonForgeSettingsType GetType() const override;
#endif // WITH_EDITOR

	TSubclassOf<UDungeonBuilder> GetBuilderClass() const { return BuilderClass; }

	//~Begin IDungeonForgeTransformToolHandler Interface
	virtual bool GetTransformToolTransform_Implementation(FTransform& OutTransform) const override;
	virtual void SetTransformToolTransform_Implementation(const FTransform& NewTransform, bool bInteractionEnded) override;
	virtual bool SupportsTransformToolRotation_Implementation() const override { return true; }
	//~End IDungeonForgeTransformToolHandler Interface

	//~Begin IDungeonForgeRandomizeToolHandler Interface
	virtual void RandomizeSeed_Implementation() const override;
	//~End IDungeonForgeRandomizeToolHandler Interface
	
protected:
	virtual TArray<FDungeonForgePinProperties> InputPinProperties() const override;
	virtual TArray<FDungeonForgePinProperties> OutputPinProperties() const override;
	virtual FDungeonForgeElementPtr CreateElement() const override;

protected:
#if WITH_EDITORONLY_DATA
	FName NodeName;
	FText NodeTitle;
	FText NodeTooltipText;
#endif // WITH_EDITORONLY_DATA
};

class FDungeonForgeCreateDungeonChunkElement : public IDungeonForgeElement {
protected:
	virtual bool ExecuteImpl(const FDungeonForgeContextPtr& Context) const override;

private:
	static void ProcessBuildTransformOverride(const FDungeonForgeContextPtr& Context, FTransform& OutOverride);
};


UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeCreateGridChunkSettings : public UDungeonForgeCreateDungeonChunkBaseSettings {
	GENERATED_BODY()
public:
	UDungeonForgeCreateGridChunkSettings();
};


UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeCreateCellFlowChunkSettings : public UDungeonForgeCreateDungeonChunkBaseSettings {
	GENERATED_BODY()
public:
	UDungeonForgeCreateCellFlowChunkSettings();
};


UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeCreateGridFlowChunkSettings : public UDungeonForgeCreateDungeonChunkBaseSettings {
	GENERATED_BODY()
public:
	UDungeonForgeCreateGridFlowChunkSettings();
};


UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeCreateCityChunkSettings : public UDungeonForgeCreateDungeonChunkBaseSettings {
	GENERATED_BODY()
public:
	UDungeonForgeCreateCityChunkSettings();
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeCreateFloorPlanSettings : public UDungeonForgeCreateDungeonChunkBaseSettings {
	GENERATED_BODY()
public:
	UDungeonForgeCreateFloorPlanSettings();
};


///////////////////////// Mask Filters

UCLASS()
class UCellFlowLayoutTaskCreateCellsGridFilter_ForgeSurface : public UCellFlowLayoutTaskCreateCellsGridFilter {
	GENERATED_BODY()
public:
	virtual void Filter(const FTransform& InTransform, const FVector& InGridSize, const FIntPoint& InTilemapSize, TArray<int>& OutCellTileMap) const override;

public:
	TWeakObjectPtr<const class UDungeonForgeTextureData> TextureData;
};