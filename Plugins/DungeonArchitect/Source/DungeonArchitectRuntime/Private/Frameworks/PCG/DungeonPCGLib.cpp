//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/PCG/DungeonPCGLib.h"

#include "Core/Dungeon.h"
#include "Core/Markers/DungeonMarker.h"
#include "Frameworks/PCG/DungeonPCGAttribute.h"

#include "Data/PCGPointData.h"
#include "Helpers/PCGHelpers.h"
#include "Metadata/PCGMetadataAttributeTpl.h"
#include "PCGComponent.h"
#include "PCGContext.h"

void FDungeonPCGLib::WriteMarkersToPCGPointData(const TArray<FDungeonMarkerInstance>& InMarkers, UPCGPointData* InPointData) {
	FPCGMetadataAttribute<int32>* AttributeID = FDungeonPCGAttributes::ID.FindOrAdd(InPointData);
	FPCGMetadataAttribute<FName>* AttributeMarkerName = FDungeonPCGAttributes::MarkerName.FindOrAdd(InPointData);
	
	TArray<FPCGPoint> PCGPoints;
	for (const FDungeonMarkerInstance& Marker : InMarkers) {
		FPCGPoint Point;
		Point.Transform = Marker.Transform;
		Point.BoundsMin = -FVector::One() * 50;
		Point.BoundsMax = FVector::One() * 50;
		Point.MetadataEntry = InPointData->Metadata->AddEntry();
		Point.Seed = PCGHelpers::ComputeSeedFromPosition(Marker.Transform.GetLocation());
		AttributeID->SetValue(Point.MetadataEntry, Marker.Id);
		AttributeMarkerName->SetValue(Point.MetadataEntry, FName(Marker.MarkerName));
		PCGPoints.Add(Point);
	}
		
	InPointData->SetPoints(PCGPoints);
}

void FDungeonPCGLib::ReadMarkersFromPCGPointData(const UPCGPointData* PointData, TArray<FDungeonMarkerInstance>& OutMarkers) {
	OutMarkers.Reset();
	
	const FPCGMetadataAttribute<int32>* AttributeID = FDungeonPCGAttributes::ID.GetConst(PointData);
	const FPCGMetadataAttribute<FName>* AttributeMarkerName = FDungeonPCGAttributes::MarkerName.GetConst(PointData);
	
	for (const FPCGPoint& Point : PointData->GetPoints()) {
		FDungeonMarkerInstance& MarkerInfo = OutMarkers.AddDefaulted_GetRef();
		MarkerInfo.Transform = Point.Transform;
		if (AttributeID) {
			MarkerInfo.Id = AttributeID->GetValue(Point.MetadataEntry);
		}
		if (AttributeMarkerName) {
			MarkerInfo.MarkerName = AttributeMarkerName->GetValue(Point.MetadataEntry).ToString();
		}
	}
}


UDungeonCanvasComponent* FDungeonPCGLib::GetCanvasComponent(const FPCGContext* InContext) {
	UDungeonCanvasComponent* CanvasComponent{};
	if (const UPCGComponent* SourceComponent = Cast<UPCGComponent>(InContext->ExecutionSource.Get())) {
		if (ADungeon* Dungeon = Cast<ADungeon>(SourceComponent->GetOwner())) {
			CanvasComponent = Cast<UDungeonCanvasComponent>(Dungeon->GetComponentByClass(UDungeonCanvasComponent::StaticClass()));
		}
	}
	return CanvasComponent;
}

