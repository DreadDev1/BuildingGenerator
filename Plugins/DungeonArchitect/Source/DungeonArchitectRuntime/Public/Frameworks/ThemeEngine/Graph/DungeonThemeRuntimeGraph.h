//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/ThemeEngine/Graph/DungeonThemeGraphNodeData.h"

struct FDungeonThemeCompiledGraph;

enum class EDungeonThemeNodeType : uint8 {
    Marker,
    MarkerEmitter,
    Visual
};

struct FDungeonThemeRuntimeNode {
    FGuid NodeGuid;
    EDungeonThemeNodeType NodeType;
    TArray<FDungeonThemeRuntimeNode*> Children;
    
    virtual ~FDungeonThemeRuntimeNode() = default;
    virtual float GetExecutionOrder() const = 0;

protected:
    explicit FDungeonThemeRuntimeNode(EDungeonThemeNodeType InType) : NodeType(InType) {}
};

// Marker node
struct FDungeonThemeRuntimeMarkerNode : FDungeonThemeRuntimeNode {
    const FDungeonThemeMarkerNodeData& NodeData;

    explicit FDungeonThemeRuntimeMarkerNode(const FDungeonThemeMarkerNodeData& InNodeData)
        : FDungeonThemeRuntimeNode(EDungeonThemeNodeType::Marker)
        , NodeData(InNodeData)
    {
        NodeGuid = InNodeData.NodeGuid;
    }
    virtual float GetExecutionOrder() const override { return 0; }
};

// Marker emitter node
struct FDungeonThemeRuntimeMarkerEmitterNode : FDungeonThemeRuntimeNode {
    const FDungeonThemeMarkerEmitterNodeData& NodeData;
    FDungeonThemeRuntimeMarkerNode* TargetMarker{};

    explicit FDungeonThemeRuntimeMarkerEmitterNode(const FDungeonThemeMarkerEmitterNodeData& InNodeData)
        : FDungeonThemeRuntimeNode(EDungeonThemeNodeType::MarkerEmitter)
        , NodeData(InNodeData)
    {
        NodeGuid = InNodeData.NodeGuid;
    }
    virtual float GetExecutionOrder() const override { return NodeData.ExecutionOrder; }
};

// Visual node
struct FDungeonThemeRuntimeVisualNode : FDungeonThemeRuntimeNode {
    const FDungeonThemeVisualNodeData& NodeData;

    explicit FDungeonThemeRuntimeVisualNode(const FDungeonThemeVisualNodeData& InNodeData)
        : FDungeonThemeRuntimeNode(EDungeonThemeNodeType::Visual)
        , NodeData(InNodeData)
    {
        NodeGuid = InNodeData.NodeGuid;
    }
    virtual float GetExecutionOrder() const override { return NodeData.ExecutionOrder; }
};

class FDungeonThemeRuntimeGraph {
public:
    FDungeonThemeRuntimeGraph(const FDungeonThemeCompiledGraph& InCompiledGraph);
    ~FDungeonThemeRuntimeGraph();

    TArray<FDungeonThemeRuntimeMarkerNode*> GetAllMarkerNodes() const;
    FDungeonThemeRuntimeMarkerNode* GetMarkerNodeByName(const FString& InName) const;
    FDungeonThemeRuntimeNode* GetNode(const FGuid& InNodeGuid) const;
    
    /** Check if we have a marker with this node and child nodes attached to it */
    bool HasMarkerTree(const FString& InMarkerName) const;

    void TraverseVisualNodes(const FString& InMarkerName, const TFunction<void(const FDungeonThemeVisualNodeData&)>& Visit) const;
    static void TraverseVisualNodes(const FDungeonThemeRuntimeMarkerNode* InNode, const TFunction<void(const FDungeonThemeVisualNodeData&)>& Visit);

private:
    static void TraverseVisualNodesImpl(const FDungeonThemeRuntimeNode* InNode, const TSet<FGuid>& InVisited, const TFunction<void(const FDungeonThemeVisualNodeData&)>& VisitNode);
    
private:
    TMap<FGuid, FDungeonThemeRuntimeNode*> AllNodes;
    TMap<FString, FDungeonThemeRuntimeMarkerNode*> NameToMarkerNodeMap;
};