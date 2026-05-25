//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/PCG/Nodes/PCGNodeFilterMarkerPoints.h"

#include "Core/Dungeon.h"
#include "Frameworks/PCG/DungeonPCGAttribute.h"
#include "Frameworks/PCG/DungeonPCGLib.h"
#include "Frameworks/PCG/Nodes/PCGNodeContext.h"

#include "Data/PCGPointData.h"
#include "Helpers/PCGAsync.h"
#include "PCGComponent.h"

#define LOCTEXT_NAMESPACE "PCGFilterDungeonMarkerPointsSettings"

///////////////////////////// UPCGGetDungeonDataSettings /////////////////////////////
UPCGFilterDungeonMarkerPointsSettings::UPCGFilterDungeonMarkerPointsSettings() {
}

bool UPCGFilterDungeonMarkerPointsSettings::UseSeed() const {
	return true;
}

#if WITH_EDITOR
FText UPCGFilterDungeonMarkerPointsSettings::GetDefaultNodeTitle() const {
	return LOCTEXT("NodeTitle", "Filter Dungeon Markers Points");
}

FText UPCGFilterDungeonMarkerPointsSettings::GetNodeTooltipText() const {
	return LOCTEXT("DungeonDataNodeTooltip", "Filter by marker names");
}
#endif // WITH_EDITOR

TArray<FPCGPinProperties> UPCGFilterDungeonMarkerPointsSettings::InputPinProperties() const {
	TArray<FPCGPinProperties> PinProperties;
	FPCGPinProperties& InputPinProperty = PinProperties.Emplace_GetRef(PCGPinConstants::DefaultInputLabel,
		EPCGDataType::Point,
		/*bInAllowMultipleConnections=*/false,
		/*bAllowMultipleData=*/false,
		LOCTEXT("OutputPinTooltip", "All input will be directly filtered to the output pin."));

	InputPinProperty.SetRequiredPin();

	return PinProperties;
}

TArray<FPCGPinProperties> UPCGFilterDungeonMarkerPointsSettings::OutputPinProperties() const {
	TArray<FPCGPinProperties> Properties;
	Properties.Emplace(TEXT("Dungeon Markers"), EPCGDataType::Point, /*bInAllowMultipleConnections =*/ false, /*bAllowMultipleData =*/ false);
	return Properties;
}

FPCGElementPtr UPCGFilterDungeonMarkerPointsSettings::CreateElement() const {
	return MakeShared<FPCGFilterDungeonMarkerDataElement>();
}


///////////////////////////// FPCGGetDungeonDataElement /////////////////////////////
FPCGContext* FPCGFilterDungeonMarkerDataElement::CreateContext() {
	return new FPCGDungeonDataContext();
}

bool FPCGFilterDungeonMarkerDataElement::PrepareDataInternal(FPCGContext* InContext) const {
	return true;
}

bool FPCGFilterDungeonMarkerDataElement::ExecuteInternal(FPCGContext* Context) const {
    const UPCGFilterDungeonMarkerPointsSettings* Settings = Context->GetInputSettings<UPCGFilterDungeonMarkerPointsSettings>();
    check(Settings);

	TArray<FPCGTaggedData> Inputs = Context->InputData.GetAllInputs();
	TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;

	
	// TODO: make parallel loop
	for (const FPCGTaggedData& Input : Inputs) {
		FPCGTaggedData& Output = Outputs.Add_GetRef(Input);
		if (!Input.Data || Cast<UPCGSpatialData>(Input.Data) == nullptr) {
			PCGE_LOG(Error, GraphAndLog, LOCTEXT("InvalidInputData", "Invalid input data"));
			continue;
		}

		const UPCGPointData* OriginalData = Cast<UPCGSpatialData>(Input.Data)->ToPointData(Context);

		if (!OriginalData) {
			PCGE_LOG(Error, GraphAndLog, LOCTEXT("NoPointDataInInput", "Unable to get point data from input"));
			continue;
		}

		const TArray<FPCGPoint>& Points = OriginalData->GetPoints();
		
		UPCGPointData* FilteredData = NewObject<UPCGPointData>();
		FilteredData->InitializeFromData(OriginalData);
		TArray<FPCGPoint>& FilteredPoints = FilteredData->GetMutablePoints();

		const FPCGMetadataAttribute<FName>* AttributeMarkerName = FDungeonPCGAttributes::MarkerName.GetConst(FilteredData);
		if (!AttributeMarkerName) {
			PCGE_LOG(Error, GraphAndLog, LOCTEXT("NoMarkerNameAttribute", "Marker name attribute not found in input data"));
			continue;
		}

		
		FPCGAsync::AsyncPointProcessing(Context, Points.Num(), FilteredPoints, [&](int32 Index, FPCGPoint& OutPoint)
		{
			const FPCGPoint& Point = Points[Index];
            FName MarkerName = AttributeMarkerName->GetValue(Point.MetadataEntry);

			bool bKeepPoint = Settings->MarkerNamesToKeep.Contains(MarkerName);
			if (Settings->bInvertFilter) {
				bKeepPoint = !bKeepPoint;
			}

			if (bKeepPoint) {
				OutPoint = Point;
				return true;
			}
			else {
				return false;
			}
		});

		Output.Data = FilteredData;
		
		PCGE_LOG(Verbose, LogOnly, FText::Format(LOCTEXT("GenerationInfo", "Generated {0} points out of {1} source points"), FilteredPoints.Num(), Points.Num()));
	}
	
	return true;
}


#undef LOCTEXT_NAMESPACE


