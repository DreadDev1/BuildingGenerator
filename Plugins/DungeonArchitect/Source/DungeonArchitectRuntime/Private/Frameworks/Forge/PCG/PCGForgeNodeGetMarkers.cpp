//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/PCG/PCGForgeNodeGetMarkers.h"

#include "Frameworks/Forge/Data/DungeonForgeMarkerData.h"
#include "Frameworks/Forge/PCG/DungeonForgePCGConstants.h"
#include "Frameworks/PCG/DungeonPCGLib.h"

#include "Data/PCGPointData.h"
#include "Helpers/PCGGraphParametersHelpers.h"
#include "PCGComponent.h"
#include "PCGContext.h"
#include "PCGGraph.h"

#define LOCTEXT_NAMESPACE "PCGForgeNodeGetMarkersSettings"

///////////////////////////// UPCGForgeNodeGetMarkersSettings /////////////////////////////
UPCGForgeNodeGetMarkersSettings::UPCGForgeNodeGetMarkersSettings() {
}

bool UPCGForgeNodeGetMarkersSettings::UseSeed() const {
	return true;
}

#if WITH_EDITOR
FText UPCGForgeNodeGetMarkersSettings::GetDefaultNodeTitle() const {
	return LOCTEXT("NodeTitle", "Get Dungeon Forge Markers");
}

FText UPCGForgeNodeGetMarkersSettings::GetNodeTooltipText() const {
	return LOCTEXT("DungeonDataNodeTooltip", "Get the dungeon forge marker points as PCG point data");
}
#endif // WITH_EDITOR

TArray<FPCGPinProperties> UPCGForgeNodeGetMarkersSettings::InputPinProperties() const {
	return TArray<FPCGPinProperties>{};
}

TArray<FPCGPinProperties> UPCGForgeNodeGetMarkersSettings::OutputPinProperties() const {
	TArray<FPCGPinProperties> Properties;
	Properties.Emplace(TEXT("Dungeon Markers"), EPCGDataType::Point, /*bInAllowMultipleConnections =*/ false, /*bAllowMultipleData =*/ false);
	return Properties;
}

FPCGElementPtr UPCGForgeNodeGetMarkersSettings::CreateElement() const {
	return MakeShared<FPCGGetDungeonForgeMarkerDataElement>();
}


///////////////////////////// FPCGGetDungeonDataElement /////////////////////////////
bool FPCGGetDungeonForgeMarkerDataElement::ExecuteInternal(FPCGContext* InContext) const {
	check(InContext);
	
	const UPCGComponent* SourceComponent = Cast<UPCGComponent>(InContext->ExecutionSource.Get());
	
	if (!SourceComponent) {
		return true;
	}
	
	const UPCGForgeNodeGetMarkersSettings* Settings = InContext->GetInputSettings<UPCGForgeNodeGetMarkersSettings>();
	check(Settings);
    
	TArray<FPCGTaggedData>& Outputs = InContext->OutputData.TaggedData;
	UPCGPointData* OutPointData = NewObject<UPCGPointData>();
	Outputs.Emplace_GetRef().Data = OutPointData;

	if (UPCGGraphInstance* GraphInstance = SourceComponent->GetGraphInstance()) {
		UObject* ParamObject = UPCGGraphParametersHelpers::GetObjectParameter(GraphInstance, DungeonForgePCGConstants::ParameterMarkers);
		if (UDungeonForgeMarkerData* MarkerData = Cast<UDungeonForgeMarkerData>(ParamObject)) {
			FDungeonPCGLib::WriteMarkersToPCGPointData(MarkerData->Markers, OutPointData);
		}
	}

	return true;
}


#undef LOCTEXT_NAMESPACE

