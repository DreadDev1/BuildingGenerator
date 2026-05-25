//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Elements/DungeonForgeApplyPCG.h"

#include "Core/Dungeon.h"
#include "Core/Utils/DungeonLog.h"
#include "Frameworks/Forge/Data/DungeonForgeLayoutData.h"
#include "Frameworks/Forge/Data/DungeonForgeLayoutTextureData.h"
#include "Frameworks/Forge/Data/DungeonForgeMarkerData.h"
#include "Frameworks/Forge/DungeonForgeResourceManager.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"
#include "Frameworks/Forge/PCG/DungeonForgePCGConstants.h"
#include "Frameworks/PCG/DungeonPCGLib.h"

#include "Data/PCGPointData.h"
#include "Helpers/PCGGraphParametersHelpers.h"
#include "PCGGraph.h"

#define LOCTEXT_NAMESPACE "DungeonForgeCreateFloorTextures"

//////////////////////////// UDungeonForgeApplyPCGSettings ////////////////////////////

#if WITH_EDITOR
FName UDungeonForgeApplyPCGSettings::GetDefaultNodeName() const {
	return TEXT("ApplyPCG");
}

FText UDungeonForgeApplyPCGSettings::GetDefaultNodeTitle() const {
	return LOCTEXT("NodeTitle", "Apply PCG");
}

FText UDungeonForgeApplyPCGSettings::GetNodeTooltipText() const {
	return LOCTEXT("NodeTooltip", "Apply PCG graph on the input");
}

EDungeonForgeSettingsType UDungeonForgeApplyPCGSettings::GetType() const {
	return EDungeonForgeSettingsType::Generic;
}
#endif // WITH_EDITOR

TArray<FDungeonForgePinProperties> UDungeonForgeApplyPCGSettings::InputPinProperties() const {
	TArray<FDungeonForgePinProperties> PinProperties;
	PinProperties.Emplace(DungeonForgePinConstants::MarkersLabel, EDungeonForgeDataType::Marker);
	PinProperties.Emplace(DungeonForgePinConstants::DungeonLayoutLabel, EDungeonForgeDataType::DungeonLayout);
	PinProperties.Emplace(DungeonForgePinConstants::DungeonLayoutTexturesLabel, EDungeonForgeDataType::DungeonLayoutTextures);
	return PinProperties;
}

TArray<FDungeonForgePinProperties> UDungeonForgeApplyPCGSettings::OutputPinProperties() const {
	TArray<FDungeonForgePinProperties> PinProperties;
	PinProperties.Emplace(DungeonForgePinConstants::MarkersLabel, EDungeonForgeDataType::Marker);
	return PinProperties;
}

FDungeonForgeElementPtr UDungeonForgeApplyPCGSettings::CreateElement() const {
	return MakeShared<FDungeonForgeApplyPCGElement>(); 
}

//////////////////////////// FDungeonForgeApplyPCGElement ////////////////////////////
bool FDungeonForgeApplyPCGElement::ExecuteImpl(const FDungeonForgeContextPtr& InContext) const {
	FDungeonForgeApplyPCGContextPtr Context = StaticCastSharedPtr<FDungeonForgeApplyPCGContext>(InContext);
	TSharedPtr<const FDungeonForgeGraphExecutionState> ExecState = Context->ExecutionState.Pin();
	if (!ExecState.IsValid()) {
		UE_LOG(LogDungeonForge, Error, TEXT("FDungeonForgeApplyPCGElement: Invalid execution state."));
		return true;
	}
	
	if (!Context->Node.IsValid() || !ExecState->ResourceManager.IsValid()) {
		UE_LOG(LogDungeonForge, Error, TEXT("FDungeonForgeApplyPCGElement: Stale node state"));
		return true;
	}

	UDungeonForgeApplyPCGSettings* NodeSettings = Cast<UDungeonForgeApplyPCGSettings>(Context->Node->GetSettings());
	if (!NodeSettings) {
		UE_LOG(LogDungeonForge, Error, TEXT("FDungeonForgeApplyPCGElement: Invalid settings state"));
		return true;
	}

	if (!NodeSettings || !NodeSettings->PCGGraph) {
		UE_LOG(LogDungeonForge, Error, TEXT("FDungeonForgeApplyPCGElement: No PCG graph assigned"));
		return true;
	}

	ADungeon* ForgeDungeon = ExecState->Dungeon.Get();
	UWorld* World = ForgeDungeon ? ForgeDungeon->GetWorld() : nullptr;
	if (!World) {
		UE_LOG(LogDungeonForge, Error, TEXT("FDungeonForgeApplyPCGElement: Invalid World state"));
		return true;
	}
	// Create a PCG Component to run the graph
	UPCGComponent* PCGComponent = NewObject<UPCGComponent>(ForgeDungeon);
	PCGComponent->RegisterComponent();
	PCGComponent->SetGraph(NodeSettings->PCGGraph);
	ForgeDungeon->AddInstanceComponent(PCGComponent);
	Context->PCGComponent = PCGComponent;

	UObject* Outer = ExecState->ResourceManager.Get();
	const UDungeonForgeMarkerData* MarkerData = Context->InputData.GetInputByPinOfType<UDungeonForgeMarkerData>(DungeonForgePinConstants::MarkersLabel);
	const UDungeonForgeLayoutData* LayoutData = Context->InputData.GetInputByPinOfType<UDungeonForgeLayoutData>(DungeonForgePinConstants::DungeonLayoutLabel);
	const UDungeonForgeLayoutTextureData* LayoutTextureData = Context->InputData.GetInputByPinOfType<UDungeonForgeLayoutTextureData>(DungeonForgePinConstants::DungeonLayoutTexturesLabel);

	if (UPCGGraphInstance* GraphInstance = PCGComponent->GetGraphInstance()) {
		if (MarkerData) {
			UPCGGraphParametersHelpers::SetObjectParameter(GraphInstance, DungeonForgePCGConstants::ParameterMarkers, StaticDuplicateObject(MarkerData, Outer));
		}
		if (LayoutData) {
			UPCGGraphParametersHelpers::SetObjectParameter(GraphInstance, DungeonForgePCGConstants::ParameterLayout, StaticDuplicateObject(LayoutData, Outer));
		}
		if (LayoutTextureData) {
			UPCGGraphParametersHelpers::SetObjectParameter(GraphInstance, DungeonForgePCGConstants::ParameterLayoutTextures, StaticDuplicateObject(LayoutTextureData, Outer));
		}
	}
	
	// Generate the PCG graph
	PCGComponent->Generate(true);

	if (IsGenerationComplete(Context)) {
		HandleGenerationComplete(Context);
		return true;
	}
	return false;
}

bool FDungeonForgeApplyPCGElement::TickImpl(const FDungeonForgeContextPtr& InContext, float DeltaTime) const {
	FDungeonForgeApplyPCGContextPtr Context = StaticCastSharedPtr<FDungeonForgeApplyPCGContext>(InContext);
	if (IsGenerationComplete(Context)) {
		HandleGenerationComplete(Context);
		return true;
	}
	return false;
}

FDungeonForgeContextPtr FDungeonForgeApplyPCGElement::CreateContext() {
	return MakeShared<FDungeonForgeApplyPCGContext, ESPMode::ThreadSafe>();
}

void FDungeonForgeApplyPCGElement::HandleGenerationComplete(const FDungeonForgeApplyPCGContextPtr& Context) {
	TSharedPtr<const FDungeonForgeGraphExecutionState> ExecState = Context->ExecutionState.Pin();
	
	
	if (Context->PCGComponent.IsValid() && ExecState.IsValid() && ExecState->ResourceManager.IsValid()) {
		UDungeonForgeMarkerData* OutputMarkerData = NewObject<UDungeonForgeMarkerData>(ExecState->ResourceManager.Get());
		// Get the data and convert it to marker list. 
		const FPCGDataCollection& GeneratedOutput = Context->PCGComponent->GetGeneratedGraphOutput();
		for (const FPCGTaggedData& TaggedData : GeneratedOutput.TaggedData) {
			if (TaggedData.Pin == TEXT("Out")) {
				if (const UPCGPointData* PointData = Cast<UPCGPointData>(TaggedData.Data)) {
					FDungeonPCGLib::ReadMarkersFromPCGPointData(PointData, OutputMarkerData->Markers);
				}
			}
		}

		// Copy the input marker data properties over to the output
		if (const UDungeonForgeMarkerData* InputMarkerData = Context->InputData.GetInputByPinOfType<UDungeonForgeMarkerData>(DungeonForgePinConstants::MarkersLabel)) {
			OutputMarkerData->ChunkId = Context->Node->NodeGuid;
			OutputMarkerData->DungeonBuilder = InputMarkerData->DungeonBuilder;
			OutputMarkerData->DungeonModel = InputMarkerData->DungeonModel;
			OutputMarkerData->DungeonConfig = InputMarkerData->DungeonConfig;
		}
		
		// Set the output data on the pin
		{
			FDungeonForgeTaggedData OutputMarkerPinData;
			OutputMarkerPinData.Pin = DungeonForgePinConstants::MarkersLabel;
			OutputMarkerPinData.Data = OutputMarkerData;
			Context->OutputData.AddData(OutputMarkerPinData);	
		}
	}
}

bool FDungeonForgeApplyPCGElement::IsGenerationComplete(const FDungeonForgeApplyPCGContextPtr& Context) {
	if  (!Context->PCGComponent.IsValid()) {
		return true;
	}
	return !Context->PCGComponent->IsGenerating() && !Context->PCGComponent->IsCleaningUp();
}

#undef LOCTEXT_NAMESPACE

