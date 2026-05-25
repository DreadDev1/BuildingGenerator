//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/PCG/Nodes/PCGNodeMergeMarkerPoints.h"

#include "Core/Dungeon.h"
#include "Frameworks/PCG/DungeonPCGAttribute.h"
#include "Frameworks/PCG/DungeonPCGLib.h"
#include "Frameworks/PCG/Nodes/PCGNodeContext.h"

#include "Data/PCGPointData.h"
#include "Helpers/PCGAsync.h"
#include "PCGComponent.h"

#define LOCTEXT_NAMESPACE "PCGMergeDungeonMarkerPointsSettings"

///////////////////////////// UPCGGetDungeonDataSettings /////////////////////////////
UPCGMergeDungeonMarkerPointsSettings::UPCGMergeDungeonMarkerPointsSettings() {
}

bool UPCGMergeDungeonMarkerPointsSettings::UseSeed() const {
	return true;
}

#if WITH_EDITOR
FText UPCGMergeDungeonMarkerPointsSettings::GetDefaultNodeTitle() const {
	return LOCTEXT("NodeTitle", "Merge Dungeon Markers Points");
}

FText UPCGMergeDungeonMarkerPointsSettings::GetNodeTooltipText() const {
	return LOCTEXT("DungeonDataNodeTooltip", "Merge Dungeon Marker Points");
}
#endif // WITH_EDITOR

TArray<FPCGPinProperties> UPCGMergeDungeonMarkerPointsSettings::InputPinProperties() const {
	/*
	TArray<FPCGPinProperties> PinProperties;
	FPCGPinProperties& InputPinProperty = PinProperties.Emplace_GetRef(PCGPinConstants::DefaultInputLabel,
		EPCGDataType::Point,
		false,
		false,
		LOCTEXT("OutputPinTooltip", "All input will be directly merged to the output pin."));

	InputPinProperty.SetRequiredPin();

	return PinProperties;
	*/
	return Super::DefaultPointInputPinProperties();
}

TArray<FPCGPinProperties> UPCGMergeDungeonMarkerPointsSettings::OutputPinProperties() const {
	TArray<FPCGPinProperties> PinProperties;
	PinProperties.Emplace(PCGPinConstants::DefaultOutputLabel, EPCGDataType::Point, false, false);

	return PinProperties;
}

FPCGElementPtr UPCGMergeDungeonMarkerPointsSettings::CreateElement() const {
	return MakeShared<FPCGMergeDungeonMarkerDataElement>();
}


///////////////////////////// FPCGGetDungeonDataElement /////////////////////////////
FPCGContext* FPCGMergeDungeonMarkerDataElement::CreateContext() {
	return new FPCGDungeonDataContext();
}

bool FPCGMergeDungeonMarkerDataElement::PrepareDataInternal(FPCGContext* InContext) const {
	return true;
}

bool FPCGMergeDungeonMarkerDataElement::ExecuteInternal(FPCGContext* Context) const {
    const UPCGMergeDungeonMarkerPointsSettings* Settings = Context->GetInputSettings<UPCGMergeDungeonMarkerPointsSettings>();
    check(Settings);

	TArray<FPCGTaggedData> Inputs = Context->InputData.GetAllInputs();
	TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;

	if (Inputs.Num() > 0) {
		FPCGTaggedData& Output = Outputs.Add_GetRef(Inputs[0]);
		UPCGPointData* OutputPointData{};
		for (const FPCGTaggedData& Input : Inputs) {
			const UPCGPointData* InputPointData = Cast<UPCGSpatialData>(Input.Data)->ToPointData(Context);

			if (!OutputPointData) {
				OutputPointData = NewObject<UPCGPointData>();
				OutputPointData->InitializeFromData(InputPointData);
				FDungeonPCGAttributes::ID.FindOrAdd(OutputPointData);
				FDungeonPCGAttributes::SDF.FindOrAdd(OutputPointData);
				FDungeonPCGAttributes::MarkerName.FindOrAdd(OutputPointData);
				FDungeonPCGAttributes::GroundHeight.FindOrAdd(OutputPointData);
				
				Output.Data = OutputPointData;
			}
			else {
				OutputPointData->InitializeFromData(InputPointData);
			}

			//OutputPointData->Metadata->AddAttributes(InputPointData->Metadata);

			const FPCGMetadataAttribute<int>* InputAttribID = FDungeonPCGAttributes::ID.GetConst(InputPointData);
			FPCGMetadataAttribute<int>* OutputAttribID = FDungeonPCGAttributes::ID.Get(OutputPointData);
			
			const FPCGMetadataAttribute<FName>* InputAttribMarkerName = FDungeonPCGAttributes::MarkerName.GetConst(InputPointData);
			FPCGMetadataAttribute<FName>* OutputAttribMarkerName = FDungeonPCGAttributes::MarkerName.Get(OutputPointData);
			
			const FPCGMetadataAttribute<float>* InputAttribSDF = FDungeonPCGAttributes::SDF.GetConst(InputPointData);
			FPCGMetadataAttribute<float>* OutputAttribSDF = FDungeonPCGAttributes::SDF.Get(OutputPointData);
			
			const FPCGMetadataAttribute<float>* InputAttribGroundHeight = FDungeonPCGAttributes::GroundHeight.GetConst(InputPointData);
			FPCGMetadataAttribute<float>* OutputAttribGroundHeight = FDungeonPCGAttributes::GroundHeight.Get(OutputPointData);

			TArray<FPCGPoint>& OutputPoints = OutputPointData->GetMutablePoints();
			for (const FPCGPoint& InputPoint : InputPointData->GetPoints()) {
				FPCGPoint OutputPoint = InputPoint;
				OutputPoint.MetadataEntry = OutputPointData->Metadata->AddEntry();

				if (InputAttribID && OutputAttribID) {
					OutputAttribID->SetValue(OutputPoint.MetadataEntry, InputAttribID->GetValue(InputPoint.MetadataEntry));
				}
				
				if (InputAttribMarkerName && OutputAttribMarkerName) {
					OutputAttribMarkerName->SetValue(OutputPoint.MetadataEntry, InputAttribMarkerName->GetValue(InputPoint.MetadataEntry));
				}
				
				if (InputAttribSDF && OutputAttribSDF) {
					OutputAttribSDF->SetValue(OutputPoint.MetadataEntry, InputAttribSDF->GetValue(InputPoint.MetadataEntry));
				}

				if (InputAttribGroundHeight && OutputAttribGroundHeight) {
					OutputAttribGroundHeight->SetValue(OutputPoint.MetadataEntry, InputAttribGroundHeight->GetValue(InputPoint.MetadataEntry));
				}
				
				OutputPoints.Add(OutputPoint);
			}
		}
	}
	
	return true;
}


#undef LOCTEXT_NAMESPACE



