//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

class UEdGraphNode;

struct FLayeredGraphLayoutConfig {
    float DepthDistance = 150;
    float SiblingDistance = 100;
    bool bHorizontalLayout = true;
};

typedef TSharedPtr<struct FLayeredLayoutNode> FLayeredLayoutNodePtr;

struct FLayeredLayoutNode {
    FGuid NodeId;
    TArray<FLayeredLayoutNodePtr> OutgoingNodes;
    TArray<FLayeredLayoutNodePtr> IncomingNodes;

    FLayeredLayoutNodePtr GetParent() const {
        return IncomingNodes.Num() > 0
            ? IncomingNodes[0]
            : nullptr;
    }

    void SetParent(const FLayeredLayoutNodePtr& InParent) {
        IncomingNodes = InParent.IsValid()
            ? TArray<FLayeredLayoutNodePtr>{ InParent }
            : TArray<FLayeredLayoutNodePtr>{};
    }
    
    float X = 0;
    int32 Depth = 0;

    float Mod = 0;
};

class FDALayeredLayoutLib {
public:
    typedef TMap<int, float> ContourMap_t;

    static void PerformLayout(const FLayeredLayoutNodePtr& RootNode) {
        TagNodeLevels(RootNode, 0);
        CalculateInitialX(RootNode, nullptr);
        CalculateFinalX(RootNode, 0);
    }
    
private:
    
    static void CalculateFinalX(const FLayeredLayoutNodePtr& Node, float TotalMod) {
        TSet<FLayeredLayoutNodePtr> Visited;
        CalculateFinalXRecursive(Node, TotalMod, Visited);
    }
    
    static void CalculateInitialX(const FLayeredLayoutNodePtr& Node, const FLayeredLayoutNodePtr& LeftSibling) {
        TSet<FLayeredLayoutNodePtr> Visited;
        CalculateInitialXRecursive(Node, LeftSibling, Visited);
    }
    
    static void TagNodeLevels(const FLayeredLayoutNodePtr& Node, int32 Depth) {
        TSet<FLayeredLayoutNodePtr> Visited;
        TagNodeLevelsRecursive(Node, Depth, Visited);
    }

    static void ResolveConflicts(const FLayeredLayoutNodePtr& Node) {
        float ShiftValue = 0.0f;
        float MinDistance = 1.0f;

        ContourMap_t NodeContour;
        GetLeftContour(Node, 0, NodeContour);

        TArray<int32> NodeLevels;
        NodeContour.GenerateKeyArray(NodeLevels);
        NodeLevels.Sort();

        FLayeredLayoutNodePtr Sibling = GetLeftMostSibling(Node);

        while (Sibling.IsValid() && Sibling != Node) {
            ContourMap_t SiblingContour;
            GetRightContour(Sibling, 0, SiblingContour);

            TArray<int32> SiblingLevels;
            SiblingContour.GenerateKeyArray(SiblingLevels);
            SiblingLevels.Sort();

            int32 MaxNodeLevel = NodeLevels.Last();
            int32 MaxSiblingLevel = SiblingLevels.Last();

            int32 StartLevel = Node->Depth + 1;
            int32 EndLevel = FMath::Min(MaxNodeLevel, MaxSiblingLevel);
            for (int32 Level = StartLevel; Level <= EndLevel; Level++) {
                float Distance = NodeContour[Level] - SiblingContour[Level];
                if (Distance + ShiftValue < MinDistance) {
                    ShiftValue = MinDistance - Distance;
                }
            }

            if (ShiftValue > 0) {
                Node->X += ShiftValue;
                Node->Mod += ShiftValue;

                ShiftValue = 0;
            }

            Sibling = GetNextSibling(Sibling);
        }
    }
    
    static void CalculateFinalXRecursive(const FLayeredLayoutNodePtr& Node, float TotalMod, TSet<FLayeredLayoutNodePtr>& Visited) {
        if (Visited.Contains(Node)) {
            return;
        }
        Visited.Add(Node);
        
        Node->X += TotalMod;

        for (FLayeredLayoutNodePtr Child : Node->OutgoingNodes) {
            CalculateFinalXRecursive(Child, TotalMod + Node->Mod, Visited);
        }
    }
    
    static void CalculateInitialXRecursive(const FLayeredLayoutNodePtr& Node, const FLayeredLayoutNodePtr& LeftSibling, TSet<FLayeredLayoutNodePtr>& Visited) {
        if (Visited.Contains(Node)) {
            return;
        }
        Visited.Add(Node);
        
        FLayeredLayoutNodePtr LeftChild;
        for (FLayeredLayoutNodePtr Child : Node->OutgoingNodes) {
            CalculateInitialXRecursive(Child, LeftChild, Visited);
            LeftChild = Child;
        }

        bool bIsLeftMost = !LeftSibling.IsValid();
        bool bIsLeaf = Node->OutgoingNodes.Num() == 0;

        if (bIsLeaf) {
            if (bIsLeftMost) {
                Node->X = 0;
            }
            else {
                Node->X = LeftSibling->X + 1;
            }
        }
        else if (Node->OutgoingNodes.Num() == 1) {
            if (bIsLeftMost) {
                Node->X = Node->OutgoingNodes[0]->X;
            }
            else {
                Node->X = LeftSibling->X + 1;
                Node->Mod = Node->X - Node->OutgoingNodes[0]->X;
            }
        }
        else {
            float LeftX = Node->OutgoingNodes[0]->X;
            float RightX = Node->OutgoingNodes.Last()->X;
            float MidX = (LeftX + RightX) / 2.0f;

            if (bIsLeftMost) {
                Node->X = MidX;
            }
            else {
                Node->X = LeftSibling->X + 1;
                Node->Mod = Node->X - MidX;
            }
        }

        if (!bIsLeaf && !bIsLeftMost) {
            ResolveConflicts(Node);
        }
    }

    static void TagNodeLevelsRecursive(const FLayeredLayoutNodePtr& Node, int32 Depth, TSet<FLayeredLayoutNodePtr>& Visited) {
        if (Visited.Contains(Node)) {
            return;
        }
        Visited.Add(Node);
        Node->Depth = Depth;
        for (FLayeredLayoutNodePtr Child : Node->OutgoingNodes) {
            TagNodeLevelsRecursive(Child, Depth + 1, Visited);
        }
    }
    
    static float FindLargestX(const TArray<FLayeredLayoutNodePtr>& Nodes) {
        if (Nodes.Num() == 0) return 0;
        float Largest = -MIN_flt;
        for (FLayeredLayoutNodePtr Node : Nodes) {
            Largest = FMath::Max(Largest, Node->X);
        }
        return Largest;
    }

    static void GetLeftContour(const FLayeredLayoutNodePtr& Node, float ModSum, ContourMap_t& ContourMap) {
        if (!ContourMap.Contains(Node->Depth)) {
            ContourMap.Add(Node->Depth, Node->X + ModSum);
        }
        else {
            ContourMap[Node->Depth] = FMath::Min(ContourMap[Node->Depth], Node->X + ModSum);
        }

        for (FLayeredLayoutNodePtr Child : Node->OutgoingNodes) {
            GetLeftContour(Child, ModSum + Node->Mod, ContourMap);
        }
    }

    static void GetRightContour(const FLayeredLayoutNodePtr& Node, float ModSum, ContourMap_t& ContourMap) {
        if (!ContourMap.Contains(Node->Depth)) {
            ContourMap.Add(Node->Depth, Node->X + ModSum);
        }
        else {
            ContourMap[Node->Depth] = FMath::Max(ContourMap[Node->Depth], Node->X + ModSum);
        }

        for (FLayeredLayoutNodePtr Child : Node->OutgoingNodes) {
            GetRightContour(Child, ModSum + Node->Mod, ContourMap);
        }
    }

    static FLayeredLayoutNodePtr GetLeftMostSibling(const FLayeredLayoutNodePtr& Node) {
        if (!Node.IsValid() || !Node->GetParent().IsValid()) {
            return nullptr;
        }

        return Node->GetParent()->OutgoingNodes[0];
    }

    static FLayeredLayoutNodePtr GetNextSibling(const FLayeredLayoutNodePtr& Node) {
        if (!Node.IsValid() || !Node->GetParent().IsValid()) {
            return nullptr;
        }

        int32 NodeIdx = Node->GetParent()->OutgoingNodes.Find(Node);
        if (NodeIdx == INDEX_NONE || NodeIdx == Node->GetParent()->OutgoingNodes.Num() - 1) {
            return nullptr;
        }

        return Node->GetParent()->OutgoingNodes[NodeIdx + 1];
    }

};

