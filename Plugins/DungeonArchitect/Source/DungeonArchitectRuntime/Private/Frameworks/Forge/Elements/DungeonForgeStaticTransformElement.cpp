//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Elements/DungeonForgeStaticTransformElement.h"

#include "Core/Utils/DungeonLog.h"
#include "Frameworks/Forge/Data/DungeonForgePointData.h"
#include "Frameworks/Forge/DungeonForgeContext.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"

#define LOCTEXT_NAMESPACE "DungeonForgeStaticTransformSettings"

#if WITH_EDITOR
FName UDungeonForgeStaticTransformSettings::GetDefaultNodeName() const {
	return TEXT("StaticTransform");
}

FText UDungeonForgeStaticTransformSettings::GetDefaultNodeTitle() const {
	return LOCTEXT("NodeTitle", "Static Transform");
}

FText UDungeonForgeStaticTransformSettings::GetNodeTooltipText() const {
	return LOCTEXT("NodeTooltip", "Make a static transform");
}

EDungeonForgeSettingsType UDungeonForgeStaticTransformSettings::GetType() const {
	return EDungeonForgeSettingsType::Spatial;
}
#endif // WITH_EDITOR

TArray<FDungeonForgePinProperties> UDungeonForgeStaticTransformSettings::InputPinProperties() const {
	return {};
}

TArray<FDungeonForgePinProperties> UDungeonForgeStaticTransformSettings::OutputPinProperties() const {
	TArray<FDungeonForgePinProperties> PinProperties;
	PinProperties.Emplace(DungeonForgeStaticLocationPins::Transform, EDungeonForgeDataType::Point);
	 
	return PinProperties;
}

FDungeonForgeElementPtr UDungeonForgeStaticTransformSettings::CreateElement() const {
	return MakeShared<FDungeonForgeStaticTransformElement>();
}

bool FDungeonForgeStaticTransformElement::ExecuteImpl(const FDungeonForgeContextPtr& Context) const {
	FDungeonForgeTaggedData LocationData;
	{
		LocationData.Pin = DungeonForgeStaticLocationPins::Transform;
		UDungeonForgePointData* PointData = NewObject<UDungeonForgePointData>();
		LocationData.Data = PointData;

		FDungeonForgePoint& Point = PointData->GetMutablePoints().AddDefaulted_GetRef();
		UDungeonForgeStaticTransformSettings* NodeSettings = Context->Node.IsValid()
				? Cast<UDungeonForgeStaticTransformSettings>(Context->Node->GetSettings())
				: nullptr;
	

		if (NodeSettings) {
			Point.Transform = NodeSettings->Transform;
		}
	}
	Context->OutputData.AddData(LocationData);

	UE_LOG(LogDungeonForge, Log, TEXT("Execute Element: StaticTransform"))
	return true;
}


#undef LOCTEXT_NAMESPACE

