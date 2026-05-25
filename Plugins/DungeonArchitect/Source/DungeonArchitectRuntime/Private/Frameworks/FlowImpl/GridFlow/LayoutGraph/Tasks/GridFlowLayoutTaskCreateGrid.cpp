//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/GridFlow/LayoutGraph/Tasks/GridFlowLayoutTaskCreateGrid.h"

#include "Builders/GridFlow/GridFlowConfig.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraph.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraphUtils.h"
#include "Frameworks/Flow/ExecGraph/FlowExecTaskAttributeMacros.h"
#include "Frameworks/FlowImpl/GridFlow/LayoutGraph/GridFlowAbstractGraph.h"
#include "Frameworks/FlowImpl/GridFlow/LayoutGraph/GridFlowAbstractGraphDomain.h"
#include "Frameworks/FlowImpl/GridFlow/Tilemap/GridFlowTilemapDomain.h"

UGridFlowLayoutTaskCreateGrid::UGridFlowLayoutTaskCreateGrid() {
    InputConstraint = EFlowExecTaskInputConstraint::NoInput;
}

////////////////////////////////////// UGridFlowTaskAbstract_CreateGrid //////////////////////////////////////
void UGridFlowLayoutTaskCreateGrid::Execute(const FFlowExecutionInput& Input, const FFlowTaskExecutionSettings& InExecSettings, FFlowExecutionOutput& Output) const {
    check(Input.IncomingNodeOutputs.Num() == 0);

    FNegationVolumeContext NegationVolumeContext{};
    {
        TSharedPtr<const FGridFlowTilemapDomain> TilemapDomain;
        for (TWeakPtr<const IFlowDomain> RegisteredDomainPtr : Input.RegisteredDomains) {
            TSharedPtr<const IFlowDomain> RegisteredDomain = RegisteredDomainPtr.Pin();
            if (RegisteredDomain.IsValid() && RegisteredDomain->GetDomainID() == FGridFlowTilemapDomain::DomainID) {
                TilemapDomain = StaticCastSharedPtr<const FGridFlowTilemapDomain>(RegisteredDomain);
                break;
            }
        }
        UGridFlowConfig* GridFlowConfig = Input.Dungeon.IsValid() ? Cast<UGridFlowConfig>(Input.Dungeon->GetConfig()) : nullptr; 
        if (TilemapDomain.IsValid() && Input.Dungeon.IsValid() && GridFlowConfig) {
            const FIntPoint ChunkSize = TilemapDomain->CachedChunkSize;
            const FIntPoint LayoutPadding = TilemapDomain->CachedLayoutPadding;
            const FVector DungeonGridSize = GridFlowConfig->GridSize;
            
            NegationVolumeContext.ChunkWorldSize = FVector(ChunkSize.X, ChunkSize.Y, 0) * DungeonGridSize;
            NegationVolumeContext.BaseOffset = FVector(LayoutPadding.X, LayoutPadding.Y, 0) * DungeonGridSize;
            if (GridFlowConfig->bAlignDungeonAtCenter) {
                FIntPoint TilemapSize = ChunkSize * GridSize + LayoutPadding;
                NegationVolumeContext.BaseOffset -= FVector(TilemapSize.X, TilemapSize.Y, 0) * 0.5f * DungeonGridSize;
            }
            NegationVolumeContext.DungeonTransform = Input.Dungeon.IsValid() ? Input.Dungeon->GetActorTransform() : FTransform::Identity;
            if (ADungeon* DungeonActor = Input.Dungeon.Get()) {
                NegationVolumeContext.NegationVolumes = FDungeonNegationVolumeState::PopulateNegationVolumeBounds(DungeonActor);
            }
            NegationVolumeContext.bEnabled = NegationVolumeContext.NegationVolumes.Num() > 0;
        }
    }
    
    // Build the graph object
    UGridFlowAbstractGraph* Graph = NewObject<UGridFlowAbstractGraph>();
    Graph->GridSize = GridSize;

    // Use a random seed that's combined with the dungeon UID, This avoids node UID collision with different dungeons on the same map 
    const FRandomStream NamingSchemeRandomStream(FFlowAbstractGraphUtils::CreateUidRandomSeed(Input.Random, Input.Dungeon));
    CreateGraph(Graph, FIntVector(GridSize.X, GridSize.Y, 1), NegationVolumeContext, NamingSchemeRandomStream);

    // Create a new state, since this will our first node
    Output.State = MakeShareable(new FFlowExecNodeState);
    Output.State->SetStateObject(UFlowAbstractGraphBase::StateTypeID, Graph);
    
    Output.ExecutionResult = EFlowTaskExecutionResult::Success;
}

void UGridFlowLayoutTaskCreateGrid::CreateGraph(UFlowAbstractGraphBase* InGraph, const FIntVector& InGridSize, const FNegationVolumeContext& InNegationVolumeContext, const FRandomStream& InNameRandomStream) {
    TMap<FIntVector, FGuid> Nodes;
    for (int z = 0; z < InGridSize.Z; z++) {
        for (int y = 0; y < InGridSize.Y; y++) {
            for (int x = 0; x < InGridSize.X; x++) {
                if (InNegationVolumeContext.bEnabled) {
                    FVector ChunkMin = FVector(x, y, z) * InNegationVolumeContext.ChunkWorldSize + InNegationVolumeContext.BaseOffset;
                    FVector ChunkMax = ChunkMin + InNegationVolumeContext.ChunkWorldSize;
                    FBox ChunkBox = FBox(ChunkMin, ChunkMax).TransformBy(InNegationVolumeContext.DungeonTransform);
                    
                    // Check this box agains all the negation volume boxes
                    bool bDiscard = false;
                    for (const FDungeonNegationVolumeState& NegationVolume : InNegationVolumeContext.NegationVolumes) {
                        if (NegationVolume.bInverse) {
                            // Anything outside this volume should be discarded.  Only chunks fully inside the negation volume are considered
                            if (!ChunkBox.IsInside(NegationVolume.Bounds)) {
                                bDiscard = true;
                                break;
                            }
                        }
                        else {
                            // Anything inside or intersecting this volume should be discarded
                            if (ChunkBox.IsInside(NegationVolume.Bounds) || ChunkBox.Intersect(NegationVolume.Bounds)) {
                                bDiscard = true;
                                break;
                            }
                        }
                    }
                    if (bDiscard) {
                        // Do not create a node here
                        continue;
                    }
                }
                
                FIntVector Coord(x, y, z);
                UFlowAbstractNode* Node = InGraph->CreateNode(InNameRandomStream);
                Node->Coord = FVector(x, y, z);
                Nodes.Add(Coord, Node->NodeId);

                if (const FGuid* PrevNodeX = Nodes.Find(FIntVector(x - 1, y, z))) {
                    InGraph->CreateLink(*PrevNodeX, Node->NodeId, InNameRandomStream);
                }

                if (const FGuid* PrevNodeY = Nodes.Find(FIntVector(x, y - 1, z))) {
                    InGraph->CreateLink(*PrevNodeY, Node->NodeId, InNameRandomStream);
                }
                if (const FGuid* PrevNodeZ = Nodes.Find(FIntVector(x, y, z - 1))) {
                    InGraph->CreateLink(*PrevNodeZ, Node->NodeId, InNameRandomStream);
                }
            }
        }
    }
}

bool UGridFlowLayoutTaskCreateGrid::GetParameter(const FString& InParameterName, FDAAttribute& OutValue) {
    FLOWTASKATTR_GET_SIZE(GridSize);

    return false;
}

bool UGridFlowLayoutTaskCreateGrid::SetParameter(const FString& InParameterName, const FDAAttribute& InValue) {
    FLOWTASKATTR_SET_SIZEI(GridSize)

    return false;
}

bool UGridFlowLayoutTaskCreateGrid::SetParameterSerialized(const FString& InParameterName, const FString& InSerializedText) {
    FLOWTASKATTR_SET_PARSE_SIZEI(GridSize)

    return false;
}

