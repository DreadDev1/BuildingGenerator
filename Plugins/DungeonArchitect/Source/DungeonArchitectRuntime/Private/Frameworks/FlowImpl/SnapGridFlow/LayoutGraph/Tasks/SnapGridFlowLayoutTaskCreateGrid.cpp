//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/SnapGridFlow/LayoutGraph/Tasks/SnapGridFlowLayoutTaskCreateGrid.h"

#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraphUtils.h"
#include "Frameworks/Flow/ExecGraph/FlowExecTaskAttributeMacros.h"
#include "Frameworks/FlowImpl/SnapGridFlow/LayoutGraph/SnapGridFlowAbstractGraph.h"
#include "Frameworks/FlowImpl/SnapGridFlow/LayoutGraph/SnapGridFlowAbstractGraphDomain.h"
#include "Frameworks/Snap/SnapGridFlow/SnapGridFlowModuleBounds.h"

////////////////////////////////////// UGridFlowTaskAbstract_CreateGrid3D //////////////////////////////////////

USnapGridFlowLayoutTaskCreateGrid::USnapGridFlowLayoutTaskCreateGrid() {
    InputConstraint = EFlowExecTaskInputConstraint::NoInput;
}

void USnapGridFlowLayoutTaskCreateGrid::Execute(const FFlowExecutionInput& Input, const FFlowTaskExecutionSettings& InExecSettings, FFlowExecutionOutput& Output) const {
    check(Input.IncomingNodeOutputs.Num() == 0);

    FNegationVolumeContext NegationVolumeContext{};
    {
        const TSharedPtr<const FSnapGridFlowAbstractGraphDomain> SGFLayoutGraphDomain = StaticCastSharedPtr<const FSnapGridFlowAbstractGraphDomain>(Input.Domain.Pin());
        const USnapGridFlowModuleDatabase* ModuleDatabase = SGFLayoutGraphDomain.IsValid() ? SGFLayoutGraphDomain->GetModuleDatabase() : nullptr;
        const USnapGridFlowModuleBoundsAsset* ModuleBoundsAsset = ModuleDatabase ? ModuleDatabase->ModuleBoundsAsset.LoadSynchronous() : nullptr;
        if (ModuleBoundsAsset) {
            NegationVolumeContext.ChunkSize = ModuleBoundsAsset->ChunkSize;
            NegationVolumeContext.DungeonTransform = Input.Dungeon.IsValid() ? Input.Dungeon->GetActorTransform() : FTransform::Identity;
            if (const ADungeon* DungeonActor = Input.Dungeon.Get()) {
                NegationVolumeContext.NegationVolumes = FDungeonNegationVolumeState::PopulateNegationVolumeBounds(DungeonActor);
            }
            NegationVolumeContext.bEnabled = NegationVolumeContext.NegationVolumes.Num() > 0;
        }
    }
    
	
    // Build the graph object
    USnapGridFlowAbstractGraph* Graph = NewObject<USnapGridFlowAbstractGraph>();
    Graph->GridSize = GridSize;
    
    // Use a random seed that's combined with the dungeon UID, This avoids node UID collision with different dungeons on the same map 
    const FRandomStream NamingSchemeRandomStream(FFlowAbstractGraphUtils::CreateUidRandomSeed(Input.Random, Input.Dungeon));
    CreateGraph(Graph, GridSize, NegationVolumeContext, NamingSchemeRandomStream);

    // Create a new state, since this will be our first node
    Output.State = MakeShareable(new FFlowExecNodeState);
    Output.State->SetStateObject(UFlowAbstractGraphBase::StateTypeID, Graph);
    
    Output.ExecutionResult = EFlowTaskExecutionResult::Success;
}

void USnapGridFlowLayoutTaskCreateGrid::CreateGraph(UFlowAbstractGraphBase* InGraph, const FIntVector& InGridSize, const FNegationVolumeContext& InNegationVolumeContext, const FRandomStream& InRandom) {
    bool bSceneContainsReversedNegationVolumes{};
    for (const FDungeonNegationVolumeState& NegationVolume : InNegationVolumeContext.NegationVolumes) {
        if (NegationVolume.bInverse) {
            bSceneContainsReversedNegationVolumes = true;
            break;
        }
    }
    
    TMap<FIntVector, FGuid> Nodes;
    for (int z = 0; z < InGridSize.Z; z++) {
        for (int y = 0; y < InGridSize.Y; y++) {
            for (int x = 0; x < InGridSize.X; x++) {
                if (InNegationVolumeContext.bEnabled) {
                    FVector ChunkMin = FVector(x - 0.5f, y - 0.5f, z - 0.5f) * InNegationVolumeContext.ChunkSize;
                    FVector ChunkMax = ChunkMin + InNegationVolumeContext.ChunkSize;
                    FBox ChunkBox = FBox(ChunkMin, ChunkMax).TransformBy(InNegationVolumeContext.DungeonTransform);
                    
                    // Check this box against all the negation volume boxes
                    bool bIsInsideNegationVolume{};
                    bool bIsInsideReversedNegationVolume{};
                    for (const FDungeonNegationVolumeState& NegationVolume : InNegationVolumeContext.NegationVolumes) {
                        if (NegationVolume.bInverse) {
                            // Anything outside this volume should be discarded.  Only chunks fully inside the negation volume are considered
                            if (NegationVolume.Bounds.IsInside(ChunkBox)) {
                                bIsInsideReversedNegationVolume = true;
                            }
                        }
                        else {
                            // Anything inside this volume should be discarded
                            if (ChunkBox.IsInside(NegationVolume.Bounds) || ChunkBox.Intersect(NegationVolume.Bounds)) {
                                bIsInsideNegationVolume = true;
                                break;
                            }
                        }
                    }

                    const bool bDiscard = bIsInsideNegationVolume || (bSceneContainsReversedNegationVolumes && !bIsInsideReversedNegationVolume);
                    if (bDiscard) {
                        // Do not create a node here
                        continue;
                    }
                }

                FIntVector Coord(x, y, z);
                UFlowAbstractNode* Node = InGraph->CreateNode(InRandom);
                Node->Coord = FVector(x, y, z);
                Nodes.Add(Coord, Node->NodeId);

                
                if (const FGuid* PrevNodeX = Nodes.Find(FIntVector(x - 1, y, z))) {
                    InGraph->CreateLink(*PrevNodeX, Node->NodeId, InRandom);
                }

                if (const FGuid* PrevNodeY = Nodes.Find(FIntVector(x, y - 1, z))) {
                    InGraph->CreateLink(*PrevNodeY, Node->NodeId, InRandom);
                }

                if (const FGuid* PrevNodeZ = Nodes.Find(FIntVector(x, y, z - 1))) {
                    InGraph->CreateLink(*PrevNodeZ, Node->NodeId, InRandom);
                }
            }
        }
    }
}

bool USnapGridFlowLayoutTaskCreateGrid::GetParameter(const FString& InParameterName, FDAAttribute& OutValue) {
    FLOWTASKATTR_GET_VECTOR(GridSize);

    return false;
    
}

bool USnapGridFlowLayoutTaskCreateGrid::SetParameter(const FString& InParameterName, const FDAAttribute& InValue) {
    FLOWTASKATTR_SET_VECTORI(GridSize)

    return false;
    
}

bool USnapGridFlowLayoutTaskCreateGrid::SetParameterSerialized(const FString& InParameterName, const FString& InSerializedText) {
    FLOWTASKATTR_SET_PARSE_VECTORI(GridSize)

    return false;
}

