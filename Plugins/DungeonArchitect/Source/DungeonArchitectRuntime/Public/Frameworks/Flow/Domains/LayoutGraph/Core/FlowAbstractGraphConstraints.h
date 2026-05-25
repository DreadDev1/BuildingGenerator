//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

struct FFlowAGPathNodeGroup;
class UFlowAbstractNode;
template<typename TNode> class TFlowAbstractGraphQuery; 
typedef TFlowAbstractGraphQuery<UFlowAbstractNode> FFlowAbstractGraphQuery;

enum class EFAGConstraintsLinkDirection : uint8 {
    Incoming,
    Outgoing
};

struct DUNGEONARCHITECTRUNTIME_API FFAGConstraintsLink {
    FFAGConstraintsLink() {}
    FFAGConstraintsLink(const UFlowAbstractNode* Node, const UFlowAbstractNode* IncomingNode, EFAGConstraintsLinkDirection InDirection)
        : Node(Node)
        , IncomingNode(IncomingNode)
        , Direction(InDirection)
    {}

    const UFlowAbstractNode* Node = nullptr;
    const UFlowAbstractNode* IncomingNode = nullptr;
    const EFAGConstraintsLinkDirection Direction = EFAGConstraintsLinkDirection::Incoming;
};

class DUNGEONARCHITECTRUNTIME_API FFlowAbstractGraphConstraints {
public:
    virtual ~FFlowAbstractGraphConstraints() {}
    
    virtual bool IsValid(const FFlowAbstractGraphQuery& InGraphQuery, const UFlowAbstractNode* Node, const TArray<const UFlowAbstractNode*>& IncomingNodes, const TArray<const UFlowAbstractNode*>& OutgoingNodes) = 0;
    virtual bool IsValid(const FFlowAbstractGraphQuery& InGraphQuery, const FFlowAGPathNodeGroup& Group, int32 PathIndex, int32 PathLength, const TArray<FFAGConstraintsLink>& IncomingNodes) = 0;
};
typedef TSharedPtr<FFlowAbstractGraphConstraints> FFlowAbstractGraphConstraintsPtr;

class DUNGEONARCHITECTRUNTIME_API FNullFlowAbstractGraphConstraints : public FFlowAbstractGraphConstraints {
public:
    virtual bool IsValid(const FFlowAbstractGraphQuery& InGraphQuery, const UFlowAbstractNode* Node, const TArray<const UFlowAbstractNode*>& IncomingNodes, const TArray<const UFlowAbstractNode*>& OutgoingNodes) override { return true; }
    virtual bool IsValid(const FFlowAbstractGraphQuery& InGraphQuery, const FFlowAGPathNodeGroup& Group, int32 PathIndex, int32 PathLength, const TArray<FFAGConstraintsLink>& IncomingNodes) override { return true; }
};
typedef TSharedPtr<FFlowAbstractGraphConstraints> FFlowAbstractGraphConstraintsPtr;

