//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/Common/DungeonThemeEngineUtils.h"

#include "Core/Utils/DungeonModelHelper.h"
#include "Frameworks/ThemeEngine/DungeonThemeEngine.h"
#include "Frameworks/ThemeEngine/Graph/DungeonThemeGraphNodeData.h"
#include "Frameworks/Voxel/Utils/VoxelLib.h"

#include "Model.h"
#include "UObject/Package.h"

FGuid FDungeonThemeEngineUtils::ParseNodeGuid(const FName& InNodeTag) {
	FString NodeTag = InNodeTag.ToString();
	if (NodeTag.StartsWith("NODE-")) {
		FString GuidString = NodeTag.RightChop(5);
		FGuid Result;
		if (FGuid::Parse(GuidString, Result)) {
			return Result;
		}
	}
	return FGuid();
}

bool FDungeonThemeEngineUtils::ProcessThemeNodeInsertion(const FDungeonThemeVisualNodeData& ThemeNode, const FTransform& InParentOffset,
    const FDungeonThemeEngineEventHandlers& EventHandlers, const FDungeonMarkerInstance& MarkerInstance,
    const FRandomStream& InRandom, int32 MarkerIdx, const ADungeon* InDungeon, FDungeonMarkerBuildData& OutNodeSpawnInfo)
{
    OutNodeSpawnInfo = {};
	bool bInsertMesh = false;
    if (ThemeNode.bUseSelectionLogic) {
        bInsertMesh = EventHandlers.PerformSelectionLogic(ThemeNode.SelectionLogics, MarkerInstance);
        if (ThemeNode.SelectionLogics.Num() == 0 || !bInsertMesh) {
            bInsertMesh = EventHandlers.PerformGenericSelectionLogic(ThemeNode.GenericSelectionLogics, MarkerInstance);
        }

        if (bInsertMesh && !ThemeNode.bLogicOverridesAffinity) {
            // The logic has selected the mesh and it doesn't override the affinity.
            // Respect the affidwnity variable and apply probability
            float Probability = InRandom.FRand();
            bInsertMesh = (Probability < ThemeNode.Probability);
        }
    }
    else {
        // Perform probability based selection logic
        float Probability = InRandom.FRand();
        bInsertMesh = (Probability < ThemeNode.Probability);
    }

    // Attach this prop to the marker
    if (bInsertMesh) {
        // Align to voxel geometry here, if requested
        FTransform MarkerTransform = MarkerInstance.Transform;

        // Place the marker on the displaced voxel geometry surface
        if (ThemeNode.bAlignToVoxelGeometry) {
            if (InDungeon && InDungeon->bCarveVoxels) {
                FDAVoxelNoiseSettings NoiseSettings = InDungeon->VoxelNoiseSettings;
                NoiseSettings.NoiseScaleVector.Z *= NoiseSettings.NoiseFloorScale;
                FVector NewLocation{};
                DA::FVoxelLib::InvertNoiseDisplacement_Picard(MarkerTransform.GetLocation(), NewLocation, NoiseSettings);
                MarkerTransform.SetLocation(NewLocation);
            }
        }
        
        FTransform Transform = ThemeNode.Offset * InParentOffset * MarkerTransform;

        // Apply transform logic, if specified
        if (ThemeNode.bUseTransformLogic) {
            FTransform LogicOffset = EventHandlers.PerformTransformLogic(ThemeNode.TransformLogics, MarkerInstance);
            FTransform OutTempTransform;
            FTransform::Multiply(&OutTempTransform, &LogicOffset, &Transform);
            Transform = OutTempTransform;
        }

        // Apply Procedural Transform logic, if specified
        if (ThemeNode.bUseProceduralTransformLogic) {
            FTransform LogicOffset = EventHandlers.PerformProceduralTransformLogic(ThemeNode.ProceduralTransformLogics, MarkerInstance);
            Transform = LogicOffset * Transform;
        }
        
        TArray<UDungeonSpawnLogic*> SpawnLogics;
        if (ThemeNode.bUseSpawnLogic && ThemeNode.SpawnLogics.Num() > 0) {
            TArray<UDungeonSpawnLogic*> SpawnLogicsSource = ThemeNode.bUseSpawnLogic ? ThemeNode.SpawnLogics : TArray<UDungeonSpawnLogic*>();
            UObject* SpawnLogicArrayOuter = GetTransientPackage();
            UDungeonModelHelper::CloneUObjectArray(SpawnLogicArrayOuter, SpawnLogicsSource, SpawnLogics);
        }

        if (ThemeNode.AssetObject) {
            //FDungeonMarkerBuildData& MarkerInfo = MarkersToEmit.AddDefaulted_GetRef();
            OutNodeSpawnInfo.Transform = Transform;
            OutNodeSpawnInfo.NodeId = ThemeNode.NodeGuid;
            OutNodeSpawnInfo.SpawnLogics = SpawnLogics;
            OutNodeSpawnInfo.TemplateObject = ThemeNode.AssetObject;
            OutNodeSpawnInfo.UserData = MarkerInstance.UserData;
            OutNodeSpawnInfo.MarkerIndex = MarkerIdx;
            OutNodeSpawnInfo.ActorTags = ThemeNode.ActorTags;
            OutNodeSpawnInfo.MarkerTags = MarkerInstance.Tags;
        }

        /*
        // Add child marker if any
        for (const FDungeonThemeMarkerEmitterNodeData& MarkerEmitter : ThemeNode.MarkerEmitters_DEPRECATED) {
            FTransform ChildTransform;
            FTransform::Multiply(&ChildTransform, &MarkerEmitter.Offset, &Transform);
            FDAThemeEngineImpl::AddMarker(Markers, MarkerEmitter.MarkerName_DEPRECATED, ChildTransform);

            // Sync the user data
            if (Markers.Num() > 0) {
                FDungeonMarkerInstance& NewMarker = Markers[Markers.Num() - 1];
                NewMarker.ClusterThemeOverride = MarkerInstance.ClusterThemeOverride;
                NewMarker.UserData = MarkerInstance.UserData;
            }
        }
        */
    }
    return bInsertMesh;
}

