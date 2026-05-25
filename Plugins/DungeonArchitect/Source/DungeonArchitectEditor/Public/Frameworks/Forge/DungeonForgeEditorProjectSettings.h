//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"

#include "EdGraph/EdGraphPin.h"
#include "Engine/DeveloperSettings.h"
#include "Templates/SubclassOf.h"
#include "DungeonForgeEditorProjectSettings.generated.h"

UCLASS(config=Editor, meta=(DisplayName="Dungeon Forge"))
class UDungeonForgeEditorProjectSettings : public UDeveloperSettings {
	GENERATED_BODY()
public:
	UDungeonForgeEditorProjectSettings(const FObjectInitializer& ObjectInitializer);
	
	FLinearColor GetColor(UDungeonForgeNodeSettings* InSettings) const;
	FLinearColor GetPinColor(const FEdGraphPinType& InPinType) const;
	
	/** Default node color */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor DefaultNodeColor;

	/** Instanced node body tint color */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor InstancedNodeBodyTintColor;

	/** Color used for World Layout nodes */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor WorldLayoutNodeColor;

	/** Color used for Dungeon Layout nodes */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor DungeonLayoutNodeColor;

	/** Color used for Literal nodes */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor LiteralNodeColor;

	/** Color used for input & output nodes */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor InputOutputNodeColor;

	/** Color used for Difference, Intersection, Projection, Union */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor SetOperationNodeColor;

	/** Color used for density remap */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor DensityOperationNodeColor;

	/** Color used for blueprints */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor BlueprintNodeColor;

	/** Color used for metadata operations */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor MetadataNodeColor;

	/** Color used for filter-like operations */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor FilterNodeColor;

	/** Color used for sampler operations */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor SamplerNodeColor;

	/** Color used for artifact-generating operations */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor SpawnerNodeColor;

	/** Color used for subgraph-like operations */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor SubgraphNodeColor;

	/** Color used for Attribute Set operations */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel, DisplayName = "Attribute Set Node Color"))
	FLinearColor ParamDataNodeColor;

	/** Color used for debug operations */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor DebugNodeColor;

	/** Color used for control flow operations */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor ControlFlowNodeColor;
	
	/** Color used for point operations */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor PointOpsNodeColor;

	/** Color used for hierarchical generation operations */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor HierarchicalGenerationNodeColor;

	/** Color used for graph parameters operations */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor GraphParametersNodeColor;

	/** Color used for reroute nodes */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor RerouteNodeColor;

	/** Color used for dynamic mesh nodes */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor DynamicMeshNodeColor;

	/** Default pin color */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor DefaultPinColor;

	/** Color used for spatial data pins */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor SpatialDataPinColor;
	
	/** Color used for concrete/simple spatial data pins */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor ConcreteDataPinColor;

	/** Color used for data pins of type Point */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor PointDataPinColor;

	/** Color used for data pins of type Spline */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor PolyLineDataPinColor;

	/** Color used for data pins of type Surface */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor SurfaceDataPinColor;

	/** Color used for data pins of type Landscape */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor LandscapeDataPinColor;

	/** Color used for data pins of type Base Texture */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor BaseTextureDataPinColor;
	
	/** Color used for data pins of type Texture */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor TextureDataPinColor;

	/** Color used for data pins of type Render Target */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor RenderTargetDataPinColor;

	/** Color used for data pins of type Volume */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor VolumeDataPinColor;

	/** Color used for data pins of type Primitive */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor PrimitiveDataPinColor;

	/** Color used for data pins of type WorldLayout of a master dungeon */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor WorldLayoutDataPinColor;

	/** Color used for data pins of type DungeonLayout of a dungeon */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor DungeonLayoutDataPinColor;
	
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor FloatDataPinColor;
	
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor Float2DataPinColor;
	
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor Float3DataPinColor;
	
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor Float4DataPinColor;

	/** Color used for data pins of type Attribute Set */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel, DisplayName = "Attribute Set Pin Color"))
	FLinearColor ParamDataPinColor;

	/** Color used for data pins of type Dynamic Mesh */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor DynamicMeshPinColor;

	/** Color used for data pins of type Marker */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor MarkerPinColor;
	
	/** Color used for other/unknown data types */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	FLinearColor UnknownDataPinColor;

	/** User-driven color overrides */
	UPROPERTY(EditAnywhere, config, Category = Node, meta = (HideAlphaChannel))
	TMap<TSubclassOf<UDungeonForgeNodeSettings>, FLinearColor> OverrideNodeColorByClass;
	
	/** Default wire thickness */
	UPROPERTY(EditAnywhere, config, Category = Wire, meta = (ClampMin="1", ClampMap="9"))
	float DefaultWireThickness = 1.5f;

	/** Hover edge size emphasis */
	UPROPERTY(EditAnywhere, config, Category = Wire, meta = (ClampMin="1", ClampMax="5"))
	float HoverEdgeEmphasis = 1.5f;

};

