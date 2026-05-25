//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Elements/DungeonForgeCreateDungeonChunk.h"

#include "Builders/CellFlow/CellFlowBuilder.h"
#include "Builders/CellFlow/CellFlowConfig.h"
#include "Builders/FloorPlan/FloorPlanBuilder.h"
#include "Builders/FloorPlan/FloorPlanConfig.h"
#include "Builders/Grid/GridDungeonBuilder.h"
#include "Builders/Grid/GridDungeonConfig.h"
#include "Builders/GridFlow/GridFlowBuilder.h"
#include "Builders/GridFlow/GridFlowConfig.h"
#include "Builders/SimpleCity/SimpleCityBuilder.h"
#include "Builders/SimpleCity/SimpleCityConfig.h"
#include "Core/Data/DungeonSpatialMapData.h"
#include "Core/Dungeon.h"
#include "Core/Utils/DungeonLog.h"
#include "Frameworks/Forge/Data/DungeonForgeLayoutData.h"
#include "Frameworks/Forge/Data/DungeonForgeMarkerData.h"
#include "Frameworks/Forge/Data/DungeonForgePointData.h"
#include "Frameworks/Forge/Data/DungeonForgeTextureData.h"
#include "Frameworks/Forge/DungeonForgeContext.h"
#include "Frameworks/Forge/DungeonForgeResourceManager.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"

#define LOCTEXT_NAMESPACE "DungeonForgeCreateDungeonChunk"


#if WITH_EDITOR
FName UDungeonForgeCreateDungeonChunkBaseSettings::GetDefaultNodeName() const {
	return NodeName;
}

FText UDungeonForgeCreateDungeonChunkBaseSettings::GetDefaultNodeTitle() const {
	return NodeTitle;
}

FText UDungeonForgeCreateDungeonChunkBaseSettings::GetNodeTooltipText() const {
	return NodeTooltipText;
}

EDungeonForgeSettingsType UDungeonForgeCreateDungeonChunkBaseSettings::GetType() const {
	return EDungeonForgeSettingsType::DungeonLayout;
}

#endif // WITH_EDITOR

bool UDungeonForgeCreateDungeonChunkBaseSettings::GetTransformToolTransform_Implementation(FTransform& OutTransform) const {
	OutTransform = BuildTransform;
	return true;
}

void UDungeonForgeCreateDungeonChunkBaseSettings::SetTransformToolTransform_Implementation(const FTransform& NewTransform, bool bInteractionEnded) {
	BuildTransform = NewTransform;
	// TODO: Update the debug draw visualization with an offset to see where the new dungeon is going to be placed
}

void UDungeonForgeCreateDungeonChunkBaseSettings::RandomizeSeed_Implementation() const {
	if (DungeonConfig) {
		DungeonConfig->Seed = FMath::Rand();
	}
}

TArray<FDungeonForgePinProperties> UDungeonForgeCreateDungeonChunkBaseSettings::InputPinProperties() const {
	TArray<FDungeonForgePinProperties> PinProperties;
	PinProperties.Emplace(DungeonForgeCreateGridChunkPins::Location, EDungeonForgeDataType::Point, false, false);
	PinProperties.Emplace(DungeonForgePinConstants::MaskLabel, EDungeonForgeDataType::Surface);
	return PinProperties;
}

TArray<FDungeonForgePinProperties> UDungeonForgeCreateDungeonChunkBaseSettings::OutputPinProperties() const {
	TArray<FDungeonForgePinProperties> PinProperties;
	PinProperties.Emplace(DungeonForgePinConstants::MarkersLabel, EDungeonForgeDataType::Marker);
	PinProperties.Emplace(DungeonForgePinConstants::DungeonLayoutLabel, EDungeonForgeDataType::DungeonLayout);
	return PinProperties;
}

FDungeonForgeElementPtr UDungeonForgeCreateDungeonChunkBaseSettings::CreateElement() const {
	return MakeShared<FDungeonForgeCreateDungeonChunkElement>();
}

/////////////////////////////// FDungeonForgeCreateGridChunkElement ///////////////////////////////
void UCellFlowLayoutTaskCreateCellsGridFilter_ForgeSurface::Filter(const FTransform& InTransform, const FVector& InGridSize, const FIntPoint& InTilemapSize, TArray<int>& OutCellTileMap) const {
	constexpr float Threshold = 0.5f;
	
	FTransform InverseDungeonTransform = InTransform.Inverse();
	if (TextureData.IsValid()) {
		for (const FDungeonForgePoint& MaskPoint : TextureData->Points) {
			if (MaskPoint.Density < Threshold) {
				// Disable this cell
				FVector LocalMaskPoint = InverseDungeonTransform.TransformPosition(MaskPoint.Transform.GetLocation());
				LocalMaskPoint /= InGridSize;
				int CellX = FMath::RoundToInt(LocalMaskPoint.X);
				int CellY = FMath::RoundToInt(LocalMaskPoint.Y);

				if (CellX < 0 || CellX >= InTilemapSize.X) {
					continue;
				}
				
				if (CellY < 0 || CellY >= InTilemapSize.Y) {
					continue;
				}

				const int Idx = CellY * InTilemapSize.X + CellX;
				OutCellTileMap[Idx] = CellFlowConstants::CELL_INVALID;
			}
		}
	}
}

bool FDungeonForgeCreateDungeonChunkElement::ExecuteImpl(const FDungeonForgeContextPtr& Context) const {
	if (!Context->Node.IsValid()) {
		UE_LOG(LogDungeonForge, Error, TEXT("CreateGridChunkElement: Stale node state"));
		return true;
	}
	
	UDungeonForgeCreateDungeonChunkBaseSettings* ChunkSettings = Cast<UDungeonForgeCreateDungeonChunkBaseSettings>(Context->Node->GetSettings());
	if (!ChunkSettings || !ChunkSettings->DungeonConfig) {
		UE_LOG(LogDungeonForge, Error, TEXT("CreateGridChunkElement: Invalid settings state"));
		return true;
	}
	
	TSharedPtr<const FDungeonForgeGraphExecutionState> ExecState = Context->ExecutionState.Pin();
	if (!ExecState.IsValid()) {
		UE_LOG(LogDungeonForge, Error, TEXT("CreateGridChunkElement: Invalid execution state."));
		return true;
	}
	
	ADungeon* ForgeDungeon = ExecState->Dungeon.Get();
	UWorld* World = ForgeDungeon ? ForgeDungeon->GetWorld() : nullptr;
	if (!World) {
		UE_LOG(LogDungeonForge, Error, TEXT("CreateGridChunkElement: Invalid World state"));
		return true;
	}

	if (!ExecState->ResourceManager.IsValid()) {
		UE_LOG(LogDungeonForge, Error, TEXT("CreateGridChunkElement: Invalid Resource manager"));
		return true;
	}
	
	FTransform BuildTransform = ChunkSettings->BuildTransform;
	ProcessBuildTransformOverride(Context, BuildTransform);
	
	TArray<const UDungeonForgeTextureData*> TextureDataList = Context->InputData.GetInputsByPinOfType<UDungeonForgeTextureData>(DungeonForgePinConstants::MaskLabel);
	check(IsInGameThread());
	
	UDungeonForgeMarkerData* MarkerData = NewObject<UDungeonForgeMarkerData>(ExecState->ResourceManager.Get());
	UDungeonBuilder* Builder = NewObject<UDungeonBuilder>(MarkerData, ChunkSettings->GetBuilderClass());
	UDungeonConfig* Config = NewObject<UDungeonConfig>(MarkerData, Builder->GetConfigClass(), NAME_None, RF_NoFlags, ChunkSettings->DungeonConfig);
	UDungeonModel* Model = NewObject<UDungeonModel>(MarkerData, Builder->GetModelClass());
	UDungeonQuery* Query = NewObject<UDungeonQuery>(MarkerData, Builder->GetQueryClass());

	FDungeonBuilderParameters BuilderParams;
	BuilderParams.bOverrideDungeonTransform = true;
	BuilderParams.DungeonTransformOverride = BuildTransform;

	// Add the mask as a builder parameter, if specified
	if (const UDungeonForgeTextureData* TextureData = Context->InputData.GetInputByPinOfType<UDungeonForgeTextureData>(DungeonForgePinConstants::MaskLabel)) {
		UCellFlowLayoutTaskCreateCellsGridFilter_ForgeSurface* CellFlowGridFilter = NewObject<UCellFlowLayoutTaskCreateCellsGridFilter_ForgeSurface>(ExecState->ResourceManager.Get());
		CellFlowGridFilter->TextureData = TextureData;
		BuilderParams.CustomParameters.SetObject(CellFlowConstants::CustomParameter_InitialGridFilterName, CellFlowGridFilter);
	}

	Builder->BuildDungeon(Model, Config, Query, World, BuilderParams);
	Builder->EmitDungeonMarkers_Implementation();

	UDungeonForgeLayoutData* LayoutData = NewObject<UDungeonForgeLayoutData>();
	Model->GenerateLayoutData(Config, LayoutData->Layout);
	LayoutData->Layout.DungeonTransform = BuildTransform;

	MarkerData->Seed = Config ? Config->Seed : 0;
	MarkerData->ChunkId = Context->Node->NodeGuid;
	MarkerData->Markers = Model->WorldMarkers;
	MarkerData->BaseTransform = BuildTransform;
	MarkerData->DungeonBuilder = Builder;
	MarkerData->DungeonModel = Model;
	MarkerData->DungeonConfig = Config;
	
	FDungeonForgeTaggedData LayoutPinData;
	LayoutPinData.Pin = DungeonForgePinConstants::DungeonLayoutLabel;
	LayoutPinData.Data = LayoutData;
	Context->OutputData.AddData(LayoutPinData);

	FDungeonForgeTaggedData MarkerPinData;
	MarkerPinData.Pin = DungeonForgePinConstants::MarkersLabel;
	MarkerPinData.Data = MarkerData;
	Context->OutputData.AddData(MarkerPinData);
	
	return true;
}

void FDungeonForgeCreateDungeonChunkElement::ProcessBuildTransformOverride(const FDungeonForgeContextPtr& Context, FTransform& OutOverride) {
	// Check is we have a location override thru the input pin
	const UDungeonForgePointData* LocationOverrideData = Context->InputData.GetInputByPinOfType<UDungeonForgePointData>(DungeonForgeCreateGridChunkPins::Location);
	if (LocationOverrideData) {
		const TArray<FDungeonForgePoint>& LocationOverridePoints = LocationOverrideData->GetPoints();
		if (LocationOverridePoints.Num() > 0) {
			OutOverride = LocationOverridePoints[0].Transform;
		}
	}
}

///////////////////////////// UDungeonForgeCreateGridChunkSettings /////////////////////////////
UDungeonForgeCreateGridChunkSettings::UDungeonForgeCreateGridChunkSettings() {
#if WITH_EDITORONLY_DATA
	NodeName = TEXT("CreateGridChunk");
	NodeTitle = LOCTEXT("NodeTitle", "Create Grid Chunk");
	NodeTooltipText = LOCTEXT("NodeTooltip", "Create a dungeon chunk using the grid builder");
#endif // WITH_EDITORONLY_DATA
	
	BuilderClass = UGridDungeonBuilder::StaticClass();
	DungeonConfig = CreateDefaultSubobject<UGridDungeonConfig>("Config");	
}

///////////////////////////// UDungeonForgeCreateGridChunkSettings /////////////////////////////
UDungeonForgeCreateCellFlowChunkSettings::UDungeonForgeCreateCellFlowChunkSettings() {
#if WITH_EDITORONLY_DATA
	NodeName = TEXT("CreateCellFlowChunk");
	NodeTitle = LOCTEXT("NodeTitle", "Create CellFlow Chunk");
	NodeTooltipText = LOCTEXT("NodeTooltip", "Create a dungeon chunk using the CellFlow builder");
#endif // WITH_EDITORONLY_DATA
	
	BuilderClass = UCellFlowBuilder::StaticClass();
	DungeonConfig = CreateDefaultSubobject<UCellFlowConfig>("Config");	
}

///////////////////////////// UDungeonForgeCreateGridFlowChunkSettings /////////////////////////////
UDungeonForgeCreateGridFlowChunkSettings::UDungeonForgeCreateGridFlowChunkSettings() {
#if WITH_EDITORONLY_DATA
	NodeName = TEXT("CreateGridFlowChunk");
	NodeTitle = LOCTEXT("NodeTitle", "Create GridFlow Chunk");
	NodeTooltipText = LOCTEXT("NodeTooltip", "Create a dungeon chunk using the GridFlow builder");
#endif // WITH_EDITORONLY_DATA
	
	BuilderClass = UGridFlowBuilder::StaticClass();
	DungeonConfig = CreateDefaultSubobject<UGridFlowConfig>("Config");	
}

///////////////////////////// UDungeonForgeCreateGridFlowChunkSettings /////////////////////////////
UDungeonForgeCreateCityChunkSettings::UDungeonForgeCreateCityChunkSettings() {
#if WITH_EDITORONLY_DATA
	NodeName = TEXT("CreateCityChunk");
	NodeTitle = LOCTEXT("NodeTitle", "Create City Chunk");
	NodeTooltipText = LOCTEXT("NodeTooltip", "Create a dungeon chunk using the City builder");
#endif // WITH_EDITORONLY_DATA
	
	BuilderClass = USimpleCityBuilder::StaticClass();
	DungeonConfig = CreateDefaultSubobject<USimpleCityConfig>("Config");	
}

///////////////////////////// UDungeonForgeCreateFloorPlanSettings /////////////////////////////
UDungeonForgeCreateFloorPlanSettings::UDungeonForgeCreateFloorPlanSettings() {
#if WITH_EDITORONLY_DATA
	NodeName = TEXT("CreateFloorPlanChunk");
	NodeTitle = LOCTEXT("NodeTitle", "Create FloorPlan Chunk");
	NodeTooltipText = LOCTEXT("NodeTooltip", "Create a dungeon chunk using the FloorPlan builder");
#endif // WITH_EDITORONLY_DATA
	
	BuilderClass = UFloorPlanBuilder::StaticClass();
	DungeonConfig = CreateDefaultSubobject<UFloorPlanConfig>("Config");	
}

#undef LOCTEXT_NAMESPACE

