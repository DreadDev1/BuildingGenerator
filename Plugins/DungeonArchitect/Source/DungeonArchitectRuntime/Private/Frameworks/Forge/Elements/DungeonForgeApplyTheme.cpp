//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Elements/DungeonForgeApplyTheme.h"

#include "Core/DungeonConfig.h"
#include "Core/Utils/DungeonLog.h"
#include "Core/Volumes/DungeonThemeOverrideVolume.h"
#include "Frameworks/Forge/Data/DungeonForgeMarkerData.h"
#include "Frameworks/Forge/DungeonForgeContext.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"
#include "Frameworks/ThemeEngine/DungeonThemeEngine.h"
#include "Frameworks/ThemeEngine/SceneProviders/DungeonSceneCollector.h"
#include "Frameworks/ThemeEngine/SceneProviders/Providers/InstancedDungeonSceneProvider.h"

#define LOCTEXT_NAMESPACE "DungeonForgeApplyThemeSettings"

#if WITH_EDITOR
FName UDungeonForgeApplyThemeSettings::GetDefaultNodeName() const {
	return TEXT("ApplyTheme");
}

FText UDungeonForgeApplyThemeSettings::GetDefaultNodeTitle() const {
	return LOCTEXT("NodeTitle", "Apply Theme");
}

FText UDungeonForgeApplyThemeSettings::GetNodeTooltipText() const {
	return LOCTEXT("NodeTooltip", "Spawn the dungeon meshes defined in the theme");
}

EDungeonForgeSettingsType UDungeonForgeApplyThemeSettings::GetType() const {
	return EDungeonForgeSettingsType::DungeonLayout;
}
#endif // WITH_EDITOR

TArray<FDungeonForgePinProperties> UDungeonForgeApplyThemeSettings::InputPinProperties() const {
	TArray<FDungeonForgePinProperties> PinProperties;
	PinProperties.Emplace(DungeonForgePinConstants::MarkersLabel, EDungeonForgeDataType::Marker, true, true);
	return PinProperties;
}

TArray<FDungeonForgePinProperties> UDungeonForgeApplyThemeSettings::OutputPinProperties() const {
	return {};
}

FDungeonForgeElementPtr UDungeonForgeApplyThemeSettings::CreateElement() const {
	return MakeShared<FDungeonForgeApplyThemeElement>();
}

/////////////////////////////// FDungeonForgeApplyThemeElement ///////////////////////////////
bool FDungeonForgeApplyThemeElement::ExecuteImpl(const FDungeonForgeContextPtr& InContext) const {
	FDungeonForgeApplyThemeContextPtr Context = StaticCastSharedPtr<FDungeonForgeApplyThemeContext>(InContext);
	
	if (!Context->Node.IsValid()) {
		UE_LOG(LogDungeonForge, Error, TEXT("ApplyThemeElement: Stale node state"));
		return true;
	}
	
	UDungeonForgeApplyThemeSettings* NodeSettings = Cast<UDungeonForgeApplyThemeSettings>(Context->Node->GetSettings());
	if (!NodeSettings) {
		UE_LOG(LogDungeonForge, Error, TEXT("ApplyThemeElement: Invalid settings state"));
		return true;
	}

	TSharedPtr<const FDungeonForgeGraphExecutionState> ExecState = Context->ExecutionState.Pin();
	if (!ExecState.IsValid()) {
		UE_LOG(LogDungeonForge, Error, TEXT("ApplyThemeElement: Invalid execution state."));
		return true;
	}
	
	ADungeon* ForgeDungeon = ExecState->Dungeon.Get();
	const FTransform DungeonTransform = ForgeDungeon ? ForgeDungeon->GetActorTransform() : FTransform::Identity;

	UWorld* World = ForgeDungeon ? ForgeDungeon->GetWorld() : nullptr;
	if (!World) {
		UE_LOG(LogDungeonForge, Error, TEXT("ApplyThemeElement: Invalid World state"));
		return true;
	}

	TArray<const UDungeonForgeMarkerData*> MarkerDataList = Context->InputData.GetInputsByPinOfType<UDungeonForgeMarkerData>(DungeonForgePinConstants::MarkersLabel);
	for (const UDungeonForgeMarkerData* MarkerData : MarkerDataList) {
		ApplyTheme(Context, NodeSettings->ThemeAsset, MarkerData);
	}

	for (const FDungeonForgeApplyThemeContext::FSceneBuilder& SceneBuilder : Context->SceneBuilders) {
		if (SceneBuilder.SceneProvider.IsValid()) {
			SceneBuilder.SceneProvider->RunGameThreadCommands(World, SceneBuilder.FrameTimeMs);
		}
	}

	const bool bComplete = !IsRunning(Context);
	return bComplete;
}

bool FDungeonForgeApplyThemeElement::TickImpl(const FDungeonForgeContextPtr& InContext, float DeltaTime) const {
	FDungeonForgeApplyThemeContextPtr Context = StaticCastSharedPtr<FDungeonForgeApplyThemeContext>(InContext);
	
	const bool bComplete = !IsRunning(Context);
	return bComplete;
}

bool FDungeonForgeApplyThemeElement::IsInstanced(const UDungeonForgeMarkerData* InMarkerData, const UDungeonForgeApplyThemeSettings* NodeSettings) {
	bool bInstanced = true;
	if (NodeSettings->bOverrideInstancedSetting) {
		bInstanced = NodeSettings->bInstanced;
	}
	else if (InMarkerData->DungeonConfig) {
		bInstanced = InMarkerData->DungeonConfig->Instanced;	
	}
	return bInstanced;
}

float FDungeonForgeApplyThemeElement::GetFrameBuildTimeMs(const UDungeonForgeMarkerData* InMarkerData, const UDungeonForgeApplyThemeSettings* NodeSettings) {
	float FrameBuildTimeMs = 12;
	if (NodeSettings->bOverrideFrameBuildTimeMs) {
		FrameBuildTimeMs = NodeSettings->FrameBuildTimeMs;
	}
	else if (InMarkerData->DungeonConfig) {
		FrameBuildTimeMs = InMarkerData->DungeonConfig->MaxBuildTimePerFrameMs;
	}
	return FrameBuildTimeMs;
}

bool FDungeonForgeApplyThemeElement::IsRunning(const FDungeonForgeApplyThemeContextPtr& InContext) {
	for (const FDungeonForgeApplyThemeContext::FSceneBuilder& SceneBuilder : InContext->SceneBuilders) {
		if (SceneBuilder.SceneProvider.IsValid()) {
			if (SceneBuilder.SceneProvider->IsRunningGameThreadCommands()) {
				return true;
			}
		}
	}
	return false;
}

TSharedPtr<FDungeonSceneProvider> FDungeonForgeApplyThemeElement::CreateSceneProvider(const UDungeonForgeMarkerData* InMarkerData, const UDungeonForgeApplyThemeSettings* InNodeSettings) {
	TSharedPtr<FDungeonSceneProvider> SceneProvider;
	const bool bInstanced = IsInstanced(InMarkerData, InNodeSettings);
	if (InMarkerData->DungeonBuilder) {
		SceneProvider = InMarkerData->DungeonBuilder->CreateSceneProvider(bInstanced);
	}
	else {
		if (bInstanced) {
			SceneProvider = MakeShared<FInstancedDungeonSceneProvider>();
		}
		else {
			SceneProvider = MakeShared<FPooledDungeonSceneProvider>();	
		}
	}
	return SceneProvider;
}

void FDungeonForgeApplyThemeElement::ApplyTheme(const FDungeonForgeApplyThemeContextPtr& InContext, const TObjectPtr<UDungeonThemeAsset>& InThemeAsset, const UDungeonForgeMarkerData* InMarkerData) const {
	if (!InMarkerData) {
		return;
	}
	
	UDungeonForgeApplyThemeSettings* NodeSettings = Cast<UDungeonForgeApplyThemeSettings>(InContext->Node->GetSettings());
	TSharedPtr<const FDungeonForgeGraphExecutionState> ExecState = InContext->ExecutionState.Pin();
	TSharedPtr<FDungeonSceneProvider> SceneProvider = CreateSceneProvider(InMarkerData, NodeSettings);
	const float FrameTimeMs = GetFrameBuildTimeMs(InMarkerData, NodeSettings);
	InContext->SceneBuilders.Add({ SceneProvider, FrameTimeMs });

	ADungeon* ForgeDungeon{};
	if (ExecState.IsValid()) {
		ForgeDungeon = ExecState->Dungeon.Get();
		if (ExecState->ChunkVisitedStates.IsValid()) {
			ExecState->ChunkVisitedStates->MarkAsVisited(InMarkerData->ChunkId);
		}
	}
	
	// Prepare the Theme Engine settings
	FDungeonThemeEngineSettings ThemeEngineSettings;
	ThemeEngineSettings.Themes = { InThemeAsset };
	ThemeEngineSettings.ClusteredThemes = {};
	ThemeEngineSettings.SceneProvider = SceneProvider;
	ThemeEngineSettings.MarkerEmitters = {};
	ThemeEngineSettings.Dungeon = ForgeDungeon;
	ThemeEngineSettings.ChunkId = InMarkerData->ChunkId;
	ThemeEngineSettings.LevelOverride = ForgeDungeon ? ForgeDungeon->GetLevel() : nullptr;

	if (InMarkerData->DungeonBuilder) {
		ThemeEngineSettings.MarkerGenerator = InMarkerData->DungeonBuilder->CreateMarkerGenProcessor(InMarkerData->BaseTransform);
	}
			
	if (ForgeDungeon) {
		ThemeEngineSettings.bRoleAuthority = ForgeDungeon->HasAuthority(); 
	}
	
	// Prepare the Theme Engine callback handlers
	FDungeonThemeEngineEventHandlers EventHandlers;
	EventHandlers.PerformSelectionLogic = [this, InMarkerData](const TArray<UDungeonSelectorLogic*>& SelectionLogics, const FDungeonMarkerInstance& InMarkerInfo) {
		if (InMarkerData->DungeonBuilder) {
			return InMarkerData->DungeonBuilder->PerformSelectionLogic(SelectionLogics, InMarkerInfo);
		}
		return false;
	};
	EventHandlers.PerformGenericSelectionLogic = [this, InMarkerData](const TArray<UGenericDungeonSelectorLogic*>& SelectionLogics, const FDungeonMarkerInstance& InMarkerInfo) {
		if (InMarkerData->DungeonBuilder) {
			return InMarkerData->DungeonBuilder->PerformGenericSelectionLogic(SelectionLogics, InMarkerInfo);
		}
		return false;
	};
	
	EventHandlers.PerformTransformLogic = [this, InMarkerData](const TArray<UDungeonTransformLogic*>& TransformLogics, const FDungeonMarkerInstance& InMarkerInfo) {
		if (InMarkerData->DungeonBuilder) {
			return InMarkerData->DungeonBuilder->PerformTransformLogic(TransformLogics, InMarkerInfo);
		}
		return FTransform::Identity;
	};

	EventHandlers.PerformProceduralTransformLogic = [this, InMarkerData](const TArray<UProceduralDungeonTransformLogic*>& ProceduralTransformLogics, const FDungeonMarkerInstance& InMarkerInfo) {
		if (InMarkerData->DungeonBuilder) {
			return InMarkerData->DungeonBuilder->PerformProceduralTransformLogic(ProceduralTransformLogics, InMarkerInfo);
		}
		return FTransform::Identity;
	};

	//EventHandlers.HandlePostMarkersEmit = [this](TArray<FDungeonMarkerBuildData>& MarkersToEmit) {
	//	DungeonUtils::FDungeonEventListenerNotifier::NotifyMarkersEmitted(Dungeon, MarkersToEmit);
	//};

	// Invoke the Theme Engine
	FRandomStream Random(InMarkerData->Seed);
	TArray<FDungeonMarkerInstance> MutableMarkers = InMarkerData->Markers;
	FDungeonThemeEngine::Apply(MutableMarkers, Random, ThemeEngineSettings, EventHandlers);
}

FDungeonForgeContextPtr FDungeonForgeApplyThemeElement::CreateContext() {
	return MakeShared<FDungeonForgeApplyThemeContext, ESPMode::ThreadSafe>();
}


#undef LOCTEXT_NAMESPACE

