//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Snap/SnapGridFlow/SnapGridFlowModuleDatabase.h"

#include "Core/Utils/MathUtils.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraphConstraints.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraphQuery.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Tasks/Lib/FlowAbstractGraphPathUtils.h"
#include "Frameworks/Snap/SnapGridFlow/SnapGridFlowStats.h"

//////////////////////////// Snap Grid Flow Module Assembly ////////////////////////////

const int32 FSGFModuleAssemblySide::INDEX_VALID_UNKNOWN = -2;

void FSGFModuleAssemblySide::Init(int32 InWidth, int32 InHeight) {
    Width = InWidth;
    Height = InHeight;
    ConnectionIndices.SetNum(Width * Height);
}

FSGFModuleAssemblySide FSGFModuleAssemblySide::Rotate90CW() const {
    FSGFModuleAssemblySide RotatedSide;
    RotatedSide.Init(Height, Width);
    for (int32 x = 0; x < Width; x++) {
        for (int32 y = 0; y < Height; y++) {
            const FSGFModuleAssemblySideCell& Cell = Get(x, y);
            const int32 xx = Height - 1 - y;
            const int32 yy = x;
            RotatedSide.SafeSet(xx, yy, Cell);
        }
    }
    
    return MoveTemp(RotatedSide);
}

void FSGFModuleAssembly::Initialize(const FIntVector& InNumChunks) {
    NumChunks = InNumChunks;
    Front.Init(NumChunks.X, NumChunks.Z);
    Left.Init(NumChunks.Y, NumChunks.Z);
    Back.Init(NumChunks.X, NumChunks.Z);
    Right.Init(NumChunks.Y, NumChunks.Z);

    Top.Init(NumChunks.X, NumChunks.Y);
    Down.Init(NumChunks.X, NumChunks.Y);
}

bool FSGFModuleAssembly::CanFit(const FSGFModuleAssembly& AssemblyToFit, TArray<FSGFModuleAssemblySideCell>& OutDoorIndices) const {
    SCOPE_CYCLE_COUNTER(STAT_SGFAsmCanFit);
    
    if (NumChunks != AssemblyToFit.NumChunks) return false;
    const FSGFModuleAssemblySide* HostSides[] = { &Front, &Left, &Back, &Right, &Top, &Down };
    const FSGFModuleAssemblySide* TargetSides[] = { &AssemblyToFit.Front, &AssemblyToFit.Left, &AssemblyToFit.Back, &AssemblyToFit.Right, &AssemblyToFit.Top, &AssemblyToFit.Down };
    OutDoorIndices.Reset();
    for (int s = 0; s < 6; s++) {
        const FSGFModuleAssemblySide& HostSide = *HostSides[s];
        const FSGFModuleAssemblySide& TargetSide = *TargetSides[s];
        check(HostSide.Width == TargetSide.Width && HostSide.Height == TargetSide.Height);
        const int32 NumEntries = HostSide.ConnectionIndices.Num();
        for (int i = 0; i < NumEntries; i++) {
            const bool bHostContainsConnection = HostSide.ConnectionIndices[i].HasConnection();
            const bool bTargetRequiresConnection = TargetSide.ConnectionIndices[i].HasConnection();
            if (bTargetRequiresConnection) {
                if (bHostContainsConnection) {
                    FSGFModuleAssemblySideCell DoorCell;
                    DoorCell.ConnectionIdx = HostSide.ConnectionIndices[i].ConnectionIdx;
                    DoorCell.NodeId = TargetSide.ConnectionIndices[i].NodeId;
                    DoorCell.LinkedNodeId = TargetSide.ConnectionIndices[i].LinkedNodeId;
                    DoorCell.Direction = HostSide.ConnectionIndices[i].Direction != ESGFModuleAssemblySideCellDirection::Unknown
                        ? HostSide.ConnectionIndices[i].Direction
                        : TargetSide.ConnectionIndices[i].Direction;
                    OutDoorIndices.Add(DoorCell);

                    // TODO: Check if the direction matches for the Entry / Exit constraint
                    
                }
                else {
                    return false;
                }
            }
        }
    }
    return true;
}

void FSGFModuleAssemblyBuilder::Build(const FVector& InChunkSize,
                                      const FSnapGridFlowModuleDatabaseItem& InModuleInfo,
                                      FSGFModuleAssembly& OutAssembly) {
    const FIntVector& NumChunks = InModuleInfo.NumChunks;
    OutAssembly = FSGFModuleAssembly();
    OutAssembly.Initialize(NumChunks);

    enum class EAssemblySide : uint8 { Unknown, Front, Left, Back, Right, Down, Top };
    
    const FVector BaseOffset = -InModuleInfo.ModuleBounds.Min;
    for (int32 ConnectionIdx = 0; ConnectionIdx < InModuleInfo.Connections.Num(); ConnectionIdx++) {
        const FSnapGridFlowModuleDatabaseConnectionInfo& ConnectionInfo = InModuleInfo.Connections[ConnectionIdx];
        FVector ConnectionLocation = ConnectionInfo.Transform.GetLocation() + BaseOffset;
        FVector LocalChunkPosition = ConnectionLocation / InChunkSize;
        const FVector& P = LocalChunkPosition;

        struct FAssemblyAttachmentDistance {
            EAssemblySide Side = EAssemblySide::Unknown;
            FVector::FReal Distance = 0;
        };

        TArray<FAssemblyAttachmentDistance> AttachmentDistances;
        AttachmentDistances.Add({ EAssemblySide::Front, FMath::Abs(P.Y) });
        AttachmentDistances.Add({ EAssemblySide::Left, FMath::Abs(P.X - NumChunks.X) });
        AttachmentDistances.Add({ EAssemblySide::Back, FMath::Abs(P.Y - NumChunks.Y) });
        AttachmentDistances.Add({ EAssemblySide::Right, FMath::Abs(P.X) });
        AttachmentDistances.Add({ EAssemblySide::Down, FMath::Abs(P.Z) });
        AttachmentDistances.Add({ EAssemblySide::Top, FMath::Abs(P.Z - NumChunks.Z) });
        
        EAssemblySide BestSide = EAssemblySide::Unknown;
        {
            constexpr float EpsilonTolerance = 1e-2f;
            float BestDistance = MAX_flt;
            for (const FAssemblyAttachmentDistance& AttachDistInfo : AttachmentDistances) {
                if (AttachDistInfo.Distance < BestDistance || FMath::IsNearlyEqual(AttachDistInfo.Distance, BestDistance, EpsilonTolerance)) {
                    if (AttachDistInfo.Side == EAssemblySide::Top || AttachDistInfo.Side == EAssemblySide::Down) {
                        if (FMath::IsNearlyEqual(BestDistance, AttachDistInfo.Distance, EpsilonTolerance)) {
                            // Give top and down lower priority. This avoids an issue where both bottom and side planes
                            // are considered valid, if the connection is placed on the bottom edge  
                            continue;
                        }
                    }
                    BestDistance = AttachDistInfo.Distance;
                    BestSide = AttachDistInfo.Side;
                }
            }
        }

        auto FloorToInt = [](float Value) {
            constexpr float EPSILON = 1e-2f;
            return FMath::FloorToInt(Value + EPSILON);
        };
        
        if (BestSide == EAssemblySide::Front) {
            // Front
            const int32 CX = FloorToInt(P.X);
            const int32 CY = FloorToInt(P.Z);
            OutAssembly.Front.SafeSet(CX, CY, ConnectionIdx);
            //UE_LOG(LogSnapGridFlowLib, Log, TEXT("FRONT: Location (%.2f, %.2f)"), P.X, P.Y);
        }
        else if (BestSide == EAssemblySide::Left) {
            // Left
            const int32 CX = FloorToInt(P.Y);
            const int32 CY = FloorToInt(P.Z);
            OutAssembly.Left.SafeSet(CX, CY, ConnectionIdx);
            //UE_LOG(LogSnapGridFlowLib, Log, TEXT("LEFT: Location (%.2f, %.2f)"), P.X, P.Y);
        }
        else if (BestSide == EAssemblySide::Back) {
            // Back
            const int32 CX = NumChunks.X - 1 - FloorToInt(P.X);
            const int32 CY = FloorToInt(P.Z);
            OutAssembly.Back.SafeSet(CX, CY, ConnectionIdx);
            //UE_LOG(LogSnapGridFlowLib, Log, TEXT("BACK: Location (%.2f, %.2f)"), P.X, P.Y);
        }
        else if (BestSide == EAssemblySide::Right) {
            // Right
            const int32 CX = NumChunks.Y - 1 - FloorToInt(P.Y);
            const int32 CY = FloorToInt(P.Z);
            OutAssembly.Right.SafeSet(CX, CY, ConnectionIdx);
            //UE_LOG(LogSnapGridFlowLib, Log, TEXT("RIGHT: Location (%.2f, %.2f)"), P.X, P.Y);
        }
        else if (BestSide == EAssemblySide::Down) {
            // Down
            const int32 CX = FloorToInt(P.X);
            const int32 CY = FloorToInt(P.Y);
            OutAssembly.Down.SafeSet(CX, CY, ConnectionIdx);
            //UE_LOG(LogSnapGridFlowLib, Log, TEXT("DOWN: Location (%.2f, %.2f)"), P.X, P.Y);
        }
        else if (BestSide == EAssemblySide::Top) {
            // Top
            const int32 CX = FloorToInt(P.X);
            const int32 CY = FloorToInt(P.Y);
            OutAssembly.Top.SafeSet(CX, CY, ConnectionIdx);
            //UE_LOG(LogSnapGridFlowLib, Log, TEXT("TOP: Location (%.2f, %.2f)"), P.X, P.Y);
        }
    }
}

void FSGFModuleAssemblyBuilder::Build(const FFlowAbstractGraphQuery& InGraphQuery, const FFlowAGPathNodeGroup& Group,
        const TArray<FFAGConstraintsLink>& IncomingNodes, FSGFModuleAssembly& OutAssembly) {
    
    SCOPE_CYCLE_COUNTER(STAT_SGFAsmBuild);
    
    FVector MinCoordF, MaxCoordF;
    for (int i = 0; i < Group.GroupNodes.Num(); i++) {
        FGuid GroupNodeId = Group.GroupNodes[i];
        UFlowAbstractNode* GroupNode = InGraphQuery.GetNode(GroupNodeId);
        if (!GroupNode) {
            GroupNode = InGraphQuery.GetSubNode(GroupNodeId);
        }
        check(GroupNode);
        
        if (i == 0) {
            MinCoordF = MaxCoordF = GroupNode->Coord; 
        }
        else {
            MinCoordF = MinCoordF.ComponentMin(GroupNode->Coord);
            MaxCoordF = MaxCoordF.ComponentMax(GroupNode->Coord);
        }
    }

    const FIntVector MinCoord = FDAMathUtils::ToIntVector(MinCoordF, true);
    const FIntVector MaxCoord = FDAMathUtils::ToIntVector(MaxCoordF, true);

    const FIntVector NumChunks = MaxCoord - MinCoord + FIntVector(1, 1, 1);
    OutAssembly.Initialize(NumChunks);
    for (const FFAGConstraintsLink& Link : IncomingNodes) {
        if (!Link.IncomingNode) continue;
        const FIntVector C = FDAMathUtils::ToIntVector(Link.Node->Coord, true) - MinCoord;
        const FIntVector IC = FDAMathUtils::ToIntVector(Link.IncomingNode->Coord, true) - MinCoord;
        check(Link.Node->MergedCompositeNodes.Num() == 0 && Link.IncomingNode->MergedCompositeNodes.Num() == 0);
        
        FSGFModuleAssemblySideCell Cell;
        Cell.ConnectionIdx = FSGFModuleAssemblySide::INDEX_VALID_UNKNOWN;
        Cell.NodeId = Link.Node ? Link.Node->NodeId : FGuid();
        Cell.LinkedNodeId = Link.IncomingNode ? Link.IncomingNode->NodeId : FGuid();
        if (Link.Direction == EFAGConstraintsLinkDirection::Incoming) {
            Cell.Direction = ESGFModuleAssemblySideCellDirection::Incoming;
        }
        else if (Link.Direction == EFAGConstraintsLinkDirection::Outgoing) {
            Cell.Direction = ESGFModuleAssemblySideCellDirection::Outgoing;
        }
        else {
            Cell.Direction = ESGFModuleAssemblySideCellDirection::Unknown;
        }
        
        if (C.Y > IC.Y) {
            // Front
            OutAssembly.Front.SafeSet(C.X, C.Z, Cell);
        }
        else if (IC.X > C.X) {
            // Left
            OutAssembly.Left.SafeSet(C.Y, C.Z, Cell);
        }
        else if (IC.Y > C.Y) {
            // Back
            OutAssembly.Back.SafeSet(NumChunks.X - 1 - C.X, C.Z, Cell);
        }
        else if (C.X > IC.X) {
            // Right
            OutAssembly.Right.SafeSet(NumChunks.Y - 1 - C.Y, C.Z, Cell);
        }
        else if (C.Z > IC.Z) {
            // Down
            OutAssembly.Down.SafeSet(C.X, C.Y, Cell);
        }
        else if (IC.Z > C.Z) {
            // Top
            OutAssembly.Top.SafeSet(C.X, C.Y, Cell);
        }
    }
}

void FSGFModuleAssemblyBuilder::Rotate90CW(const FSGFModuleAssembly& InAssembly, FSGFModuleAssembly& OutAssembly) {
    OutAssembly = FSGFModuleAssembly();
    OutAssembly.NumChunks = FIntVector(InAssembly.NumChunks.Y, InAssembly.NumChunks.X, InAssembly.NumChunks.Z);
    OutAssembly.Left = InAssembly.Front;
    OutAssembly.Back = InAssembly.Left;
    OutAssembly.Right = InAssembly.Back;
    OutAssembly.Front = InAssembly.Right;
    OutAssembly.Top = InAssembly.Top.Rotate90CW();
    OutAssembly.Down = InAssembly.Down.Rotate90CW();
}

