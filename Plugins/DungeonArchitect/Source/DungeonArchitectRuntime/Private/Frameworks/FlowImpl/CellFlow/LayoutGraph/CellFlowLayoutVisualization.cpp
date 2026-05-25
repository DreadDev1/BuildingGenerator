//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutVisualization.h"

#include "Frameworks/Flow/Utils/FlowVisLib.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Impl/CellFlowGrid.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/Meshing/MeshGeneratorDCEL.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/Meshing/MeshGeneratorGrid.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/Meshing/MeshLib.h"

#include "Components/DynamicMeshComponent.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"

ACellFlowLayoutVisualization::ACellFlowLayoutVisualization(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) {
	SceneRoot = CreateDefaultSubobject<USceneComponent>("SceneRoot");
	SetRootComponent(SceneRoot);

	DebugData = CreateDefaultSubobject<UDASceneDebugDataComponent>("DebugData");

	auto InitComponent = [this](TObjectPtr<UDynamicMeshComponent>& Component, const FName& InComponentName, bool bInCollision,
	                            const TCHAR* InMaterialName) {
		Component = CreateDefaultSubobject<UDynamicMeshComponent>(InComponentName);
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetCastShadow(true);

		UMaterialInterface* SurfaceMaterial = Cast<UMaterialInterface>(
			StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, InMaterialName));
		Component->SetMaterial(0, SurfaceMaterial ? SurfaceMaterial : UMaterial::GetDefaultMaterial(MD_Surface));
		Component->SetupAttachment(RootComponent);
	};

	InitComponent(QuadSurfaceComponent, "QuadSurfaceComponent", true,
	              TEXT("/DungeonArchitect/Core/Runtime/Features/FlowGraph/CellFlow/M_CellFlow_Quads_Inst"));
	InitComponent(VoronoiSurfaceComponent, "VoronoiSurfaceComponent", true,
	              TEXT("/DungeonArchitect/Core/Runtime/Features/FlowGraph/CellFlow/M_CellFlow_Voronoi_Inst"));
	InitComponent(LineComponent, "LineComponent", false,
	              TEXT("/DungeonArchitect/Core/Runtime/Assets/Materials/Common/M_VertexColorTranslucent"));
}


void ACellFlowLayoutVisualization::Generate(UCellFlowLayoutGraph* InLayoutGraph, UDAFlowCellGraph* InCellGraph,
                                            const UCellFlowVoronoiGraph* InVoronoiData, const FCellFlowLevelMeshGenSettings& InSettings) const
{
	using namespace UE::Geometry;

	DebugData->Data = InCellGraph->SceneDebugData;
	DebugData->RenderScale = InSettings.VisualizationScale;
	
	FFlowVisLib::FGeometry LineGeometry;

	{
		FFlowVisLib::FGeometry SurfaceGeometry;
		FCellFlowMeshGenGrid::Generate(InLayoutGraph, InCellGraph, InSettings, SurfaceGeometry, LineGeometry);
		FCellFlowLevelMeshLib::UploadGeometry(SurfaceGeometry, QuadSurfaceComponent);
	}

	{
		check(InSettings.bGeneratedMergedMesh);
		TArray<FFlowVisLib::FGeometry> SurfaceGeometries;
		FCellFlowMeshGenDCEL::Generate(InLayoutGraph, InCellGraph, InVoronoiData, InSettings, SurfaceGeometries, &LineGeometry);
		
		static const FFlowVisLib::FGeometry FallbackEmptyGeometry{};
		const FFlowVisLib::FGeometry* SurfaceGeometry = SurfaceGeometries.Num() > 0 ? &SurfaceGeometries[0] : &FallbackEmptyGeometry;
		FCellFlowLevelMeshLib::UploadGeometry(*SurfaceGeometry, VoronoiSurfaceComponent);
	}

	FCellFlowLevelMeshLib::UploadGeometry(LineGeometry, LineComponent);
}

