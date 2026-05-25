//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/FlowEditor/DomainEditors/Impl/CellFlow/Viewport3D/CellFlowDomainEdLayoutGraph3D.h"

#include "Core/Editors/FlowEditor/DomainEditors/Impl/CellFlow/Viewport3D/SCellFlowDomainEdViewport.h"
#include "Core/LevelEditor/Visualizers/SceneDebugDataComponentVisualizer.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraph.h"
#include "Frameworks/Flow/ExecGraph/FlowExecTask.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutGraph.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutGraphDomain.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutVisualization.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskCreateCellsVoronoi.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowLib.h"

#include "Engine/Blueprint.h"

#define LOCTEXT_NAMESPACE "CellFlowDomainEdLayoutGraph3D"

FCellFlowDomainEdLayoutGraph3D::FCellFlowDomainEdLayoutGraph3D() {
	constexpr float NodeRadius = 100.0f; 
	VisualizerSettings.NodeRadius = NodeRadius;;
	VisualizerSettings.LinkThickness = NodeRadius * 0.2f;
	VisualizerSettings.LinkRefThickness = VisualizerSettings.LinkThickness * 0.5f;
	VisualizerSettings.NodeSeparationDistance = FVector(400, 400, 200);
	VisualizerSettings.DisabledNodeScale = 0.6f;
	VisualizerSettings.DisabledNodeOpacity = 0.75f;
}

void FCellFlowDomainEdLayoutGraph3D::InitializeImpl(const FDomainEdInitSettings& InSettings) {
	Super::InitializeImpl(InSettings);

	if (UWorld* World = Viewport->GetWorld()) {
		// Spawn the post process volume
		const TSoftObjectPtr<UBlueprint> BP_PostProcessPath(FSoftObjectPath(TEXT("/DungeonArchitect/Core/Runtime/Features/FlowGraph/Blueprints/BP_PostProcess.BP_PostProcess")));
		if (const UBlueprint* BP_PostProcess = BP_PostProcessPath.LoadSynchronous()) {
			World->SpawnActor(BP_PostProcess->GeneratedClass);
		}
	}
}

IFlowDomainPtr FCellFlowDomainEdLayoutGraph3D::CreateDomain() const {
	return MakeShareable(new FCellFlowLayoutGraphDomain);
}

void FCellFlowDomainEdLayoutGraph3D::BuildCustomVisualization(UWorld* InWorld, const FFlowExecNodeStatePtr& State) {
	UCellFlowLayoutGraph* LayoutGraph = State->GetState<UCellFlowLayoutGraph>(UFlowAbstractGraphBase::StateTypeID);
	UDAFlowCellGraph* CellGraph = State->GetState<UDAFlowCellGraph>(UDAFlowCellGraph::StateTypeID);
	const UCellFlowVoronoiGraph* VoronoiData = State->GetState<UCellFlowVoronoiGraph>(UCellFlowVoronoiGraph::StateTypeID);
	if (!LayoutGraph || !CellGraph) return;

	const ACellFlowLayoutVisualization* Visualizer = FindActor<ACellFlowLayoutVisualization>(InWorld);
	if (!Visualizer) {
		Visualizer = InWorld->SpawnActor<ACellFlowLayoutVisualization>();
		Viewport->RegisterComponentForVisualization(MakeShared<FDASceneDebugDataComponentVisualizer>(), Visualizer->GetDebugComponent());
	}

	if (Visualizer) {
		FCellFlowLevelMeshGenSettings RenderSettings;
		RenderSettings.bRenderInactive = CellGraph->bRenderInactiveGroups;
		RenderSettings.VisualizationScale = VisualizerSettings.NodeSeparationDistance;
		RenderSettings.bGeneratedMergedMesh = true;
		RenderSettings.bApplyNoise = false;
		Visualizer->Generate(LayoutGraph, CellGraph, VoronoiData, RenderSettings);
	}
}

TSharedPtr<SFlowDomainEdViewport3D> FCellFlowDomainEdLayoutGraph3D::CreateViewport() const {
	TSharedPtr<SCellFlowDomainEdViewport> NewViewport = SNew(SCellFlowDomainEdViewport);
	NewViewport->GetViewportClient()->SetViewMode(VMI_Lit);
	return NewViewport;
}

float FCellFlowDomainEdLayoutGraph3D::GetLayoutGraphVisualizerZOffset() {
	return VisualizerSettings.NodeRadius * 1.25f;
}


#undef LOCTEXT_NAMESPACE

