//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/PCG/Nodes/PCGNodeGetMarkerPoints.h"

#include "Core/Dungeon.h"
#include "Frameworks/PCG/DungeonPCGLib.h"
#include "Frameworks/PCG/Nodes/PCGNodeContext.h"

#include "Data/PCGPointData.h"
#include "PCGComponent.h"

#define LOCTEXT_NAMESPACE "PCGGetDungeonMarkerPointsSettings"

///////////////////////////// UPCGGetDungeonDataSettings /////////////////////////////
UPCGGetDungeonMarkerPointsSettings::UPCGGetDungeonMarkerPointsSettings() {
}

bool UPCGGetDungeonMarkerPointsSettings::UseSeed() const {
	return true;
}

#if WITH_EDITOR
FText UPCGGetDungeonMarkerPointsSettings::GetDefaultNodeTitle() const {
	return LOCTEXT("NodeTitle", "Get Dungeon Markers Points");
}

FText UPCGGetDungeonMarkerPointsSettings::GetNodeTooltipText() const {
	return LOCTEXT("DungeonDataNodeTooltip", "Get the dungeon marker points as PCG point data");
}
#endif // WITH_EDITOR

TArray<FPCGPinProperties> UPCGGetDungeonMarkerPointsSettings::InputPinProperties() const {
	return TArray<FPCGPinProperties>{};
}

TArray<FPCGPinProperties> UPCGGetDungeonMarkerPointsSettings::OutputPinProperties() const {
	TArray<FPCGPinProperties> Properties;
	Properties.Emplace(TEXT("Dungeon Markers"), EPCGDataType::Point, /*bInAllowMultipleConnections =*/ false, /*bAllowMultipleData =*/ false);
	return Properties;
}

FPCGElementPtr UPCGGetDungeonMarkerPointsSettings::CreateElement() const {
	return MakeShared<FPCGGetDungeonMarkerDataElement>();
}


///////////////////////////// FPCGGetDungeonDataElement /////////////////////////////
FPCGContext* FPCGGetDungeonMarkerDataElement::CreateContext() {
	return new FPCGDungeonDataContext();
}

bool FPCGGetDungeonMarkerDataElement::PrepareDataInternal(FPCGContext* InContext) const {
	return true;
}

bool FPCGGetDungeonMarkerDataElement::ExecuteInternal(FPCGContext* InContext) const {
	FPCGDungeonDataContext* Context = static_cast<FPCGDungeonDataContext*>(InContext);
    check(Context);
	
	const UPCGComponent* SourceComponent = Cast<UPCGComponent>(Context->ExecutionSource.Get());
    if (!SourceComponent) {
    	return true;
    }


    const UPCGGetDungeonMarkerPointsSettings* Settings = Context->GetInputSettings<UPCGGetDungeonMarkerPointsSettings>();
    check(Settings);
    
    TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;
    UPCGPointData* OutPointData = NewObject<UPCGPointData>();
    Outputs.Emplace_GetRef().Data = OutPointData;

	
    if (ADungeon* Dungeon = Cast<ADungeon>(SourceComponent->GetOwner())) {
	    if (FDungeonPCGComponentCache* Cache = Dungeon->PCGCache->GetComponentCache(SourceComponent)) {
	    	const TArray<FDungeonMarkerInstance>& MarkerList = Cache->InputWorldMarkers;
	    	FDungeonPCGLib::WriteMarkersToPCGPointData(MarkerList, OutPointData);
	    }
    }

	return true;
}


#undef LOCTEXT_NAMESPACE

