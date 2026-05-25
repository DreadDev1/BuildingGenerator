//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/DungeonForgeEditorProjectSettings.h"

#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorCommon.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"

#include "EdGraph/EdGraphPin.h"

UDungeonForgeEditorProjectSettings::UDungeonForgeEditorProjectSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultNodeColor = FLinearColor(0.4f, 0.62f, 1.0f);
	InstancedNodeBodyTintColor = FLinearColor(0.5f, 0.5f, 0.5f);
	WorldLayoutNodeColor = FLinearColor(1.0f, 0.38f, 0.02f);;
	DungeonLayoutNodeColor = FLinearColor(1.0f, 0.05f, 0.05f);;
	LiteralNodeColor = FLinearColor(0.02f, 0.18f, 1.0f);
	InputOutputNodeColor = FLinearColor(1.0f, 0.0f, 0.0f);
	SetOperationNodeColor = FLinearColor(1.0f, 0.2f, 1.0f);
	DensityOperationNodeColor = FLinearColor(0.2f, 0.59f, 0.99f);
	BlueprintNodeColor = FLinearColor(0.02f, 0.18f, 1.0f);
	MetadataNodeColor = FLinearColor(1.0f, 0.99f, 0.99f);
	FilterNodeColor = FLinearColor(0.24f, 0.09f, 0.85f);
	SamplerNodeColor = FLinearColor(0.0f, 0.0f, 0.0f);
	SpawnerNodeColor = FLinearColor(0.0f, 1.0f, 0.69f);
	SubgraphNodeColor = FLinearColor(1.0f, 0.05f, 0.05f);
	ParamDataNodeColor = FLinearColor(1.0f, 0.38f, 0.02f);
	DebugNodeColor = FLinearColor(1.0f, 0.0f, 1.0f);
	ControlFlowNodeColor = FLinearColor(0.0f, 1.0f, 0.0f);
	PointOpsNodeColor = FLinearColor(0.0f, 0.04f, 0.23f);
	HierarchicalGenerationNodeColor = FLinearColor(1.0f, 0.132868f, 0.0f);
	GraphParametersNodeColor = FLinearColor::Yellow;
	RerouteNodeColor = FLinearColor(0.5f, 1.0f, 0.83f);
	DynamicMeshNodeColor = FLinearColor(0.42f, 0.25f, 1.0f);

	DefaultPinColor = FLinearColor(0.29f, 0.29f, 0.29f);
	SpatialDataPinColor = FLinearColor(1.0f, 1.0f, 1.0f);
	ConcreteDataPinColor = FLinearColor(0.45f, 0.38f, 0.96f);
	PointDataPinColor = FLinearColor(0.05f, 0.25f, 1.0f);
	PolyLineDataPinColor = FLinearColor(0.05f, 0.75f, 0.82f);
	SurfaceDataPinColor = FLinearColor(0.06f, 0.55f, 0.21f);
	LandscapeDataPinColor = FLinearColor(0.66f, 0.66f, 0.07f);
	BaseTextureDataPinColor = FLinearColor(0.81f, 0.28f, 0.22f);
	TextureDataPinColor = FLinearColor(0.79f, 0.08f, 0.01f);
	RenderTargetDataPinColor = FLinearColor(0.8f, 0.18f, 0.12f);
	VolumeDataPinColor = FLinearColor(0.79f, 0.06f, 0.5f);
	PrimitiveDataPinColor = FLinearColor(0.22f, 0.05f, 1.0f);
	WorldLayoutDataPinColor = FLinearColor(1.0f, 0.38f, 0.02f);;
	DungeonLayoutDataPinColor = FLinearColor(0.05f, 0.75f, 0.82f);;
	FloatDataPinColor = FLinearColor(0.0f, 1.0f, 0.0f);
	Float2DataPinColor = FLinearColor(0.22f, 0.05f, 1.0f);
	Float3DataPinColor = FLinearColor(0.66f, 0.66f, 0.07f);
	Float4DataPinColor = FLinearColor(0.22f, 0.05f, 1.0f);
	DynamicMeshPinColor = FLinearColor(0.42f, 0.25f, 1.0f);
	MarkerPinColor = FLinearColor::Yellow;

	ParamDataPinColor = FLinearColor(1.0f, 0.38f, 0.02f);
	UnknownDataPinColor = FLinearColor(0.3f, 0.3f, 0.3f);
}

FLinearColor UDungeonForgeEditorProjectSettings::GetColor(UDungeonForgeNodeSettings* Settings) const {
	if (!Settings)
	{
		return DefaultNodeColor;
	}

	// First: check if there's an override
	if (const FLinearColor* Override = OverrideNodeColorByClass.Find(Settings->GetClass()))
	{
		return *Override;
	}

	// Otherwise, check against the classes we know
	switch (Settings->GetType())
	{
	case EDungeonForgeSettingsType::WorldLayout:
		return WorldLayoutNodeColor;
	case EDungeonForgeSettingsType::DungeonLayout:
		return DungeonLayoutNodeColor;
	case EDungeonForgeSettingsType::Literal:
		return LiteralNodeColor;
	case EDungeonForgeSettingsType::InputOutput:
		return InputOutputNodeColor;
	case EDungeonForgeSettingsType::Spatial:
		return SetOperationNodeColor;
	case EDungeonForgeSettingsType::Density:
		return DensityOperationNodeColor;
	case EDungeonForgeSettingsType::Blueprint:
		return BlueprintNodeColor;
	case EDungeonForgeSettingsType::Metadata:
		return MetadataNodeColor;
	case EDungeonForgeSettingsType::Filter:
		return FilterNodeColor;
	case EDungeonForgeSettingsType::Sampler:
		return SamplerNodeColor;
	case EDungeonForgeSettingsType::Spawner:
		return SpawnerNodeColor;
	case EDungeonForgeSettingsType::Subgraph:
		return SubgraphNodeColor;
	case EDungeonForgeSettingsType::Debug:
		return DebugNodeColor;
	case EDungeonForgeSettingsType::Param:
		return ParamDataNodeColor;
	case EDungeonForgeSettingsType::HierarchicalGeneration:
		return HierarchicalGenerationNodeColor;
	case EDungeonForgeSettingsType::ControlFlow:
		return ControlFlowNodeColor;
	case EDungeonForgeSettingsType::PointOps:
		return PointOpsNodeColor;
	case EDungeonForgeSettingsType::GraphParameters:
		return GraphParametersNodeColor;
	case EDungeonForgeSettingsType::Reroute:
		return RerouteNodeColor;
	case EDungeonForgeSettingsType::DynamicMesh:
		return DynamicMeshNodeColor;
	case EDungeonForgeSettingsType::Generic: // falls through
	default:
		// Finally, we couldn't find any match, so return the default value
		return DefaultNodeColor;
	}
}

FLinearColor UDungeonForgeEditorProjectSettings::GetPinColor(const FEdGraphPinType& InPinType) const {
	if (InPinType.PinCategory == FDungeonForgeEditorCommon::ConcreteDataType)
	{
		// Clauses below try to pick the narrowest type possible, falling back to Spatial
		if (InPinType.PinSubCategory == FDungeonForgeEditorCommon::PointDataType)
		{
			return PointDataPinColor;
		}
		else if (InPinType.PinSubCategory == FDungeonForgeEditorCommon::PolyLineDataType)
		{
			return PolyLineDataPinColor;
		}
		else if (InPinType.PinSubCategory == FDungeonForgeEditorCommon::LandscapeDataType)
		{
			return LandscapeDataPinColor;
		}
		else if (InPinType.PinSubCategory == FDungeonForgeEditorCommon::BaseTextureDataType)
		{
			return BaseTextureDataPinColor;
		}
		else if (InPinType.PinSubCategory == FDungeonForgeEditorCommon::TextureDataType)
		{
			return TextureDataPinColor;
		}
		else if (InPinType.PinSubCategory == FDungeonForgeEditorCommon::RenderTargetDataType)
		{
			return RenderTargetDataPinColor;
		}
		else if (InPinType.PinSubCategory == FDungeonForgeEditorCommon::SurfaceDataType)
		{
			return SurfaceDataPinColor;
		}
		else if (InPinType.PinSubCategory == FDungeonForgeEditorCommon::VolumeDataType)
		{
			return VolumeDataPinColor;
		}
		else if (InPinType.PinSubCategory == FDungeonForgeEditorCommon::DynamicMeshDataType)
		{
			return DynamicMeshPinColor;
		}
		else if (InPinType.PinSubCategory == FDungeonForgeEditorCommon::MarkerDataType)
		{
			return MarkerPinColor;
		}
		else if (InPinType.PinSubCategory == FDungeonForgeEditorCommon::PrimitiveDataType)
		{
			return PrimitiveDataPinColor;
		}
		else if (InPinType.PinSubCategory == FDungeonForgeEditorCommon::WorldLayoutType)
		{
			return WorldLayoutDataPinColor;
		}
		else if (InPinType.PinSubCategory == FDungeonForgeEditorCommon::DungeonLayoutType)
		{
			return DungeonLayoutDataPinColor;
		}
		else if (InPinType.PinSubCategory == FDungeonForgeEditorCommon::FloatType)
		{
			return FloatDataPinColor;
		}
		else if (InPinType.PinSubCategory == FDungeonForgeEditorCommon::Float2Type)
		{
			return Float2DataPinColor;
		}
		else if (InPinType.PinSubCategory == FDungeonForgeEditorCommon::Float3Type)
		{
			return Float3DataPinColor;
		}
		else if (InPinType.PinSubCategory == FDungeonForgeEditorCommon::Float4Type)
		{
			return Float4DataPinColor;
		}
		else
		{
			return ConcreteDataPinColor;
		}
	}
	else if (InPinType.PinCategory == FDungeonForgeEditorCommon::SpatialDataType)
	{
		return SpatialDataPinColor;
	}
	else if (InPinType.PinCategory == FDungeonForgeEditorCommon::ParamDataType)
	{
		return ParamDataPinColor;
	}
	else if (InPinType.PinCategory == FDungeonForgeEditorCommon::OtherDataType)
	{
		return UnknownDataPinColor;
	}
	else
	{
		return DefaultPinColor;
	}
}

