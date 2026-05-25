//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/DungeonThemeEngine.h"

#include "Core/Actors/DungeonActorTemplate.h"
#include "Core/Actors/DungeonMeshList.h"
#include "Core/Markers/DungeonMarker.h"
#include "Core/Utils/Rectangle.h"
#include "Core/Volumes/DungeonThemeOverrideVolume.h"
#include "Frameworks/MarkerGenerator/MarkerGenLayer.h"
#include "Frameworks/MarkerGenerator/MarkerGenModel.h"
#include "Frameworks/ThemeEngine/Common/DungeonThemeEngineUtils.h"
#include "Frameworks/ThemeEngine/DungeonThemeAsset.h"
#include "Frameworks/ThemeEngine/Graph/DungeonThemeCompiledGraphUpgrader.h"
#include "Frameworks/ThemeEngine/Graph/DungeonThemeGraphNodeData.h"
#include "Frameworks/ThemeEngine/Graph/DungeonThemeRuntimeGraph.h"
#include "Frameworks/ThemeEngine/SceneProviders/DungeonSceneProvider.h"
#include "Frameworks/ThemeEngine/SceneProviders/DungeonSceneProviderContext.h"

#include "Engine/StaticMesh.h"
#include "Particles/ParticleSystem.h"
#include "UObject/Package.h"

typedef TMap<UDungeonThemeAsset*, TSharedPtr<FDungeonThemeRuntimeGraph>> FDungeonThemeRuntimeGraphRegistry;

class FDAThemeEngineImpl {
public:
    static void RegisterThemeRuntimeGraph(UDungeonThemeAsset* InThemeAsset, FDungeonThemeRuntimeGraphRegistry& ThemeGraphRegistry) {
        if (!InThemeAsset || ThemeGraphRegistry.Contains(InThemeAsset)) {
            // Lookup for this theme has already been built
            return;
        }

        ThemeGraphRegistry.Add(InThemeAsset, MakeShared<FDungeonThemeRuntimeGraph>(InThemeAsset->CompiledThemeGraph));
    }

    // Picks a theme from the list that has a definition for the defined marker
    static UDungeonThemeAsset* GetBestMatchedTheme(const FRandomStream& InRandom, const TArray<UDungeonThemeAsset*>& InThemes,
                                            const FDungeonMarkerInstance& InMarker, const FDungeonThemeRuntimeGraphRegistry& InThemeGraphRegistry)
    {
        TArray<UDungeonThemeAsset*> ValidThemes;
        for (UDungeonThemeAsset* Theme : InThemes) {
            if (InThemeGraphRegistry.Contains(Theme)) {
                TSharedPtr<FDungeonThemeRuntimeGraph> ThemeRuntimeGraph = InThemeGraphRegistry[Theme];
                if (ThemeRuntimeGraph.IsValid() && ThemeRuntimeGraph->HasMarkerTree(InMarker.MarkerName)) {
                    ValidThemes.Add(Theme);
                }
            }
        }
        if (ValidThemes.Num() == 0) {
            return nullptr;
        }

        const int32 Index = FMath::FloorToInt(InRandom.FRand() * ValidThemes.Num()) % ValidThemes.Num();
        return ValidThemes[Index];
    }

};

void FDungeonThemeEngine::Apply(TArray<FDungeonMarkerInstance>& Markers, const FRandomStream& InRandom,
        const FDungeonThemeEngineSettings& InSettings, const FDungeonThemeEngineEventHandlers& EventHandlers) {

    auto EmitCustomMarkers = [&InSettings](EDungeonMarkerEmitterExecStage InExecutionStage) {
        for (UDungeonMarkerEmitter* MarkerEmitter : InSettings.MarkerEmitters) {
            if (MarkerEmitter && MarkerEmitter->ExecutionStage == InExecutionStage) {
                if (InSettings.Dungeon.IsValid()) {
                    UDungeonBuilder* DungeonBuilder = InSettings.Dungeon->GetBuilder();
                    UDungeonConfig* DungeonConfig = InSettings.Dungeon->GetConfig();
                    UDungeonModel* DungeonModel = InSettings.Dungeon->GetModel();
                    UDungeonQuery* DungeonQuery = InSettings.Dungeon->GetQuery();
                    MarkerEmitter->EmitMarkers(DungeonBuilder, DungeonModel, DungeonConfig, DungeonQuery);
                }
            }
        }
    };

    EmitCustomMarkers(EDungeonMarkerEmitterExecStage::BeforePatternMatcher);

    // Upgrade the compiled theme data, if needed
    for (UDungeonThemeAsset* ThemeAsset : InSettings.Themes) {
        if (ThemeAsset && !ThemeAsset->IsLatestVersion()) {
            FDungeonThemeCompiledGraphUpgrader::Upgrade(ThemeAsset);
        }
    }
    
    // Run the Marker Generators on the existing marker list
    if (InSettings.MarkerGenerator.IsValid()) {
        for (UDungeonThemeAsset* Theme : InSettings.Themes) {
            if (Theme && Theme->MarkerGenerationModel) {
                if (Theme->MarkerGenerationModel->Layers.Num() > 0) {
                    /*
                    // Transform the markers to local dungeon space
                    FTransform DungeonTransform = InSettings.Dungeon.IsValid() ? InSettings.Dungeon->GetTransform() : FTransform::Identity;
                    FTransform DungeonInverseTransform = DungeonTransform.Inverse();
                    for (FDungeonMarkerInstance& Marker : Markers) {
                        Marker.Transform = Marker.Transform * DungeonInverseTransform;
                    }
                    */
                    
                    for (UMarkerGenLayer* MarkerGenLayer : Theme->MarkerGenerationModel->Layers) {
                        TArray<FDungeonMarkerInstance> NewMarkers;
                        if (InSettings.MarkerGenerator->Process(MarkerGenLayer, Markers, InRandom, NewMarkers)) {
                            Markers = NewMarkers;
                        }
                    }

                    /*
                    // Transform back to dungeon transform
                    for (FDungeonMarkerInstance& Marker : Markers) {
                        Marker.Transform = Marker.Transform * DungeonTransform;
                    }
                    */
                }
            }
        }
    }

    EmitCustomMarkers(EDungeonMarkerEmitterExecStage::AfterPatternMatcher);

    FDungeonThemeRuntimeGraphRegistry ThemeGraphRegistry;
    for (UDungeonThemeAsset* Theme : InSettings.Themes) {
        FDAThemeEngineImpl::RegisterThemeRuntimeGraph(Theme, ThemeGraphRegistry);
    }

    // Process the Theme Overrides
    TMap<ADungeonThemeOverrideVolume*, FBox> ThemeOverrideLocalBounds;
    for (ADungeonThemeOverrideVolume* OverrideVolume : InSettings.ThemeOverrideVolumes) {
        FBox LocalVolumeBounds = OverrideVolume->CalcLocalVolumeBounds();
        ThemeOverrideLocalBounds.Add(OverrideVolume, LocalVolumeBounds);

        // Build a lookup of the theme for faster access later on
        FDAThemeEngineImpl::RegisterThemeRuntimeGraph(OverrideVolume->ThemeOverride, ThemeGraphRegistry);
    }

    TMap<FString, FClusterThemeInfo> ClusteredThemeByName;
    for (const FClusterThemeInfo& ClusteredThemeInfo : InSettings.ClusteredThemes) {
        if (!ClusteredThemeByName.Contains(ClusteredThemeInfo.ClusterThemeName)) {
            ClusteredThemeByName.Add(ClusteredThemeInfo.ClusterThemeName, ClusteredThemeInfo);
        }

        for (UDungeonThemeAsset* Theme : ClusteredThemeInfo.Themes) {
            if (!ThemeGraphRegistry.Contains(Theme)) {
                FDAThemeEngineImpl::RegisterThemeRuntimeGraph(Theme, ThemeGraphRegistry);
            }
        }
    }

    TArray<FDungeonMarkerBuildData> NodesToSpawn;

    // when ever the theme editor emits a marker, this increments on that marker index.   Used to limit the depth to avoid unforeseen infinite loops
    TArray<int32> MarkerDepths;
    MarkerDepths.AddDefaulted(Markers.Num());

    const ADungeon* Dungeon = InSettings.Dungeon.Get();
    FTransform DungeonTransform = Dungeon ? Dungeon->GetActorTransform() : FTransform::Identity;
    FTransform InverseDungeonTransform = DungeonTransform.Inverse();
    
    // Fill up the marker with the defined mesh data 
    for (int32 MarkerIdx = 0; MarkerIdx < Markers.Num(); MarkerIdx++) {
        //const FDungeonMarkerInstance& MarkerInstance = Markers[MarkerIdx];
        const int32 MarkerInstanceDepth = MarkerDepths[MarkerIdx];

        UDungeonThemeAsset* ThemeToUse = nullptr;

        // User the overridden theme if specified
        if (!Markers[MarkerIdx].ClusterThemeOverride.IsEmpty() && ClusteredThemeByName.Contains(Markers[MarkerIdx].ClusterThemeOverride)) {
            ThemeToUse = FDAThemeEngineImpl::GetBestMatchedTheme(InRandom, ClusteredThemeByName[Markers[MarkerIdx].ClusterThemeOverride].Themes, Markers[MarkerIdx], ThemeGraphRegistry); 
        }

        if (!ThemeToUse) {
            // use the default theme list
            ThemeToUse = FDAThemeEngineImpl::GetBestMatchedTheme(InRandom, InSettings.Themes, Markers[MarkerIdx], ThemeGraphRegistry);
        }

        UDungeonThemeAsset* FallbackTheme = ThemeToUse;

        ADungeonThemeOverrideVolume* BestOverrideVolume = nullptr;
        float BestOverrideWeight = 0;

        
        // Check if this marker resides within a override volume
        for (auto& Entry : ThemeOverrideLocalBounds) {
            ADungeonThemeOverrideVolume* ThemeOverrideVolume = Entry.Key;
            FBox LocalOverrideBounds = Entry.Value;
            
            FVector Location = Markers[MarkerIdx].Transform.GetLocation();
            if (!ThemeOverrideVolume) continue;
            FVector LocalLocation = InverseDungeonTransform.TransformPosition(Location);

            bool bIntersects = LocalOverrideBounds.IsInside(LocalLocation);
            if (ThemeOverrideVolume->Reversed) {
                bIntersects = !bIntersects;
            }

            if (bIntersects) {
                if (!BestOverrideVolume || BestOverrideWeight < ThemeOverrideVolume->OverrideWeight) {
                    BestOverrideVolume = ThemeOverrideVolume;
                    BestOverrideWeight = ThemeOverrideVolume->OverrideWeight;
                }
            }
        }

        if (BestOverrideVolume) {
            ThemeToUse = BestOverrideVolume->ThemeOverride;
            if (!BestOverrideVolume->FallbackOnMissingMarkers) {
                // We do not want a fallback. So use this same theme as a fallback
                FallbackTheme = ThemeToUse;
            }
        }

        if (!ThemeToUse) continue;

        check(ThemeGraphRegistry.Contains(ThemeToUse));
        TSharedPtr<FDungeonThemeRuntimeGraph> RuntimeGraph = ThemeGraphRegistry[ThemeToUse];
        if (FallbackTheme != ThemeToUse && FallbackTheme != nullptr && !RuntimeGraph->HasMarkerTree(Markers[MarkerIdx].MarkerName)) {
            // The theme we are about to use doesn't have any nodes attached to this marker.
            // Check if we can use a fallback theme
            TSharedPtr<FDungeonThemeRuntimeGraph> FallbackGraph = ThemeGraphRegistry[FallbackTheme];
            if (FallbackGraph->HasMarkerTree(Markers[MarkerIdx].MarkerName)) {
                RuntimeGraph = FallbackGraph;
            }
        }

        /** Executes the node in the theme graph. returns true if we should continue execution along the sibling */
        TFunction<bool(FDungeonThemeRuntimeNode* InNode, const FTransform& InParentTransform)> ProcessNode;
        ProcessNode = [&](FDungeonThemeRuntimeNode* InNode, const FTransform& InParentTransform) {
            // Handle marker emitter
            if (InNode->NodeType == EDungeonThemeNodeType::MarkerEmitter) {
                FDungeonThemeRuntimeMarkerEmitterNode* EmitterNode = static_cast<FDungeonThemeRuntimeMarkerEmitterNode*>(InNode);
                if (EmitterNode->TargetMarker) {
                    //FTransform HierarchyTransform = EmitterNode->NodeData.Offset * InParentTransform;
                    //ProcessNode(EmitterNode->TargetMarker, HierarchyTransform);
                    constexpr int32 MAX_EMITTER_DEPTH = 16;
                    if (MarkerInstanceDepth < MAX_EMITTER_DEPTH) {
                        FTransform ChildTransform;
                        FTransform BaseMarkerTransform = InParentTransform * Markers[MarkerIdx].Transform;
                        FTransform::Multiply(&ChildTransform, &EmitterNode->NodeData.Offset, &BaseMarkerTransform);
                        if (FDungeonThemeRuntimeNode* MarkerNodeObj = RuntimeGraph->GetNode(EmitterNode->NodeData.MarkerNodeGuid)) {
                            if (MarkerNodeObj->NodeType == EDungeonThemeNodeType::Marker) {
                                FDungeonThemeRuntimeMarkerNode* MarkerNode = static_cast<FDungeonThemeRuntimeMarkerNode*>(MarkerNodeObj);
                                
                                FDungeonMarkerInstance& NewMarker = Markers.Emplace_GetRef();
                                NewMarker.Id = Markers.Num() - 1;
                                NewMarker.MarkerName = MarkerNode->NodeData.MarkerName;
                                NewMarker.Transform = ChildTransform;
                                NewMarker.UserData = Markers[MarkerIdx].UserData;
                                NewMarker.Tags = Markers[MarkerIdx].Tags;
                                NewMarker.ClusterThemeOverride = Markers[MarkerIdx].ClusterThemeOverride;

                                int32& NewMarkerDepth = MarkerDepths.Emplace_GetRef();
                                NewMarkerDepth = MarkerInstanceDepth + 1;
                            }
                        }
                    }
                }
                
                constexpr bool bContinueExecutionAlongSibling = true;
                return bContinueExecutionAlongSibling;
            }

            if (InNode->NodeType == EDungeonThemeNodeType::Visual) {
                FDungeonThemeRuntimeVisualNode* VisualNode = static_cast<FDungeonThemeRuntimeVisualNode*>(InNode);
                //bool bInserted = ApplyMarkerInstanceOnNode(VisualNode->NodeData, InParentTransform);
                FDungeonMarkerBuildData NodeSpawnInfo{};
                bool bInserted = FDungeonThemeEngineUtils::ProcessThemeNodeInsertion(VisualNode->NodeData, InParentTransform,
                        EventHandlers, Markers[MarkerIdx], InRandom, MarkerIdx, Dungeon, NodeSpawnInfo);
                if (bInserted) {
                    NodesToSpawn.Add(NodeSpawnInfo);
                    // Process the children, since we inserted this node into the scene
                    FTransform HierarchyTransform = VisualNode->NodeData.Offset * InParentTransform;
                    for (FDungeonThemeRuntimeNode* Child : VisualNode->Children) {
                        if (!ProcessNode(Child, HierarchyTransform)) {
                            break;
                        }
                    }
                }
                const bool bContinueExecutionAlongSibling = !bInserted || !VisualNode->NodeData.ConsumeOnAttach;
                return bContinueExecutionAlongSibling;
            }

            if (InNode->NodeType == EDungeonThemeNodeType::Marker) {
                for (FDungeonThemeRuntimeNode* Child : InNode->Children) {
                    if (!ProcessNode(Child, InParentTransform)) {
                        break;
                    }
                }
                
                constexpr bool bContinueExecutionAlongSibling = true;
                return bContinueExecutionAlongSibling;
            }

            return false;
        };

        if (FDungeonThemeRuntimeMarkerNode* MarkerNodeRT = RuntimeGraph->GetMarkerNodeByName(Markers[MarkerIdx].MarkerName)) {
            ProcessNode(MarkerNodeRT, FTransform::Identity);
        }
    }

    EventHandlers.HandlePostMarkersEmit(NodesToSpawn);

    // Create the scene build commands based on the markers emitted on the scene
    if (InSettings.SceneProvider.IsValid()) {
        InSettings.SceneProvider->OnDungeonBuildStart(InSettings.Dungeon.Get(), InSettings.ChunkId);

        for (const FDungeonMarkerBuildData& NodeSpawnInfo : NodesToSpawn) {
            FDungeonSceneProviderContext Context;
            Context.Transform = NodeSpawnInfo.Transform;
            Context.NodeId = NodeSpawnInfo.NodeId;
            for (TObjectPtr<UDungeonSpawnLogic> DungeonSpawnLogic : NodeSpawnInfo.SpawnLogics) {
                Context.SpawnLogics.Add(DungeonSpawnLogic);
            }
            Context.MarkerUserData = NodeSpawnInfo.UserData;
            Context.RandomStream = &InRandom;
            Context.Dungeon = InSettings.Dungeon;
            Context.ChunkTag = InSettings.ChunkId;
            Context.MarkerIndex = NodeSpawnInfo.MarkerIndex;
            Context.LevelOverride = InSettings.LevelOverride;
            Context.ActorTags = NodeSpawnInfo.ActorTags;

            auto AddActorFromTemplate = [&InSettings](UClass* InClassTemplate, const FDungeonSceneProviderContext& InContext, bool bCanEverAffectNavigation) {
                bool bCanSpawn = true;
                if (!InSettings.bRoleAuthority) {
                    // This is a client. Make sure we are not spawning a replicated actor
                    if (const AActor* TemplateCDO = Cast<AActor>(InClassTemplate->ClassDefaultObject)) {
                        if (TemplateCDO->GetIsReplicated()) {
                            bCanSpawn = false;
                        }
                    }
                }
            
                if (bCanSpawn) {
                    InSettings.SceneProvider->AddActorFromTemplate(InContext, InClassTemplate, bCanEverAffectNavigation);
                }
            };
            
            // Add a mesh instance, if specified
            if (UDungeonMesh* Mesh = Cast<UDungeonMesh>(NodeSpawnInfo.TemplateObject)) {
                InSettings.SceneProvider->AddStaticMesh(Context, Mesh);
            }
            else if (UDungeonMeshList* MeshList = Cast<UDungeonMeshList>(NodeSpawnInfo.TemplateObject)) {
                // Pick a random item from the list
                const int32 TotalItems = MeshList->StaticMeshes.Num() + MeshList->ActorTemplates.Num();
                if (TotalItems > 0) {
                    int Index = InRandom.RandRange(0, TotalItems - 1);
                    if (Index < MeshList->StaticMeshes.Num()) {
                        FDungeonMeshListItem& MeshItem = MeshList->StaticMeshes[Index];
                        if (UStaticMesh* RandMesh = MeshItem.StaticMesh) {
                            Context.Transform = MeshItem.Transform * Context.Transform; 
                            Context.NodeId = MeshItem.GetNodeItemGuid(NodeSpawnInfo.NodeId);
                            InSettings.SceneProvider->AddStaticMesh(Context, RandMesh, MeshList->bCanEverAffectNavigation);
                        }    
                    }
                    else {
                        Index -= MeshList->StaticMeshes.Num();
                        check(MeshList->ActorTemplates.IsValidIndex(Index));
                        FDungeonActorTemplateListItem& TemplateInfo = MeshList->ActorTemplates[Index];
                        if (TemplateInfo.ClassTemplate) {
                            Context.Transform = TemplateInfo.Transform * Context.Transform; 
                            Context.NodeId = TemplateInfo.GetNodeItemGuid(NodeSpawnInfo.NodeId);
                            AddActorFromTemplate(TemplateInfo.ClassTemplate, Context, MeshList->bCanEverAffectNavigation);
                        }
                    }
                }
            }
            else if (UPointLightComponent* Light = Cast<UPointLightComponent>(NodeSpawnInfo.TemplateObject)) {
                InSettings.SceneProvider->AddLight(Context, Light);
            }
            else if (UParticleSystem* Particle = Cast<UParticleSystem>(NodeSpawnInfo.TemplateObject)) {
                InSettings.SceneProvider->AddParticleSystem(Context, Particle);
            }
            else if (UClass* ClassTemplate = Cast<UClass>(NodeSpawnInfo.TemplateObject)) {
                AddActorFromTemplate(ClassTemplate, Context, MeshList->bCanEverAffectNavigation);
            }
            else if (UDungeonActorTemplate* ActorTemplate = Cast<UDungeonActorTemplate>(NodeSpawnInfo.TemplateObject)) {
                bool bCanSpawn = true;
                if (!InSettings.bRoleAuthority) {
                    // This is a client. Make sure we are not spawning a replicated actor
                    if (ActorTemplate->ClassTemplate) {
                        
                        if (const AActor* TemplateCDO = GetDefault<AActor>(ActorTemplate->ClassTemplate)) {
                            if (TemplateCDO->GetIsReplicated()) {
                                bCanSpawn = false;
                            }
                        }
                    }
                }
                if (bCanSpawn) {
                    InSettings.SceneProvider->AddClonedActor(Context, ActorTemplate);
                }
            }
            else {
                // Not supported.  Give the implementation an opportunity to handle it
                InSettings.SceneProvider->ProcessUnsupportedObject(NodeSpawnInfo.TemplateObject, Context);
            }
        }

        EventHandlers.HandleEmitCustomSceneObjects(*InSettings.SceneProvider);
        
        InSettings.SceneProvider->OnDungeonBuildStop(InSettings.Dungeon.Get(), InSettings.ChunkId);
    }
}

