//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Snap/SnapMap/SnapMapLibrary.h"

#include "Frameworks/GraphGrammar/GraphGrammar.h"
#include "Frameworks/GraphGrammar/Script/GrammarRuleScript.h"
#include "Frameworks/Snap/Lib/Module/SnapModuleBoundShape.h"

///////////////////////////// FSnapGraphGrammarNode /////////////////////////////
FSnapGraphGrammarNode::FSnapGraphGrammarNode(UGrammarScriptGraphNode* InGraphNode)
    : GraphNode(InGraphNode)
{
}

FGuid FSnapGraphGrammarNode::GetNodeID() const {
    return GraphNode->NodeId;
}

FName FSnapGraphGrammarNode::GetCategory() const {
    UGrammarRuleScriptGraphNode* RuleNode = Cast<UGrammarRuleScriptGraphNode>(GraphNode);
    const FName ModuleCategory = RuleNode ? RuleNode->TypeInfo->TypeName : FName();
    return ModuleCategory;
}

TArray<SnapLib::ISnapGraphNodePtr> FSnapGraphGrammarNode::GetOutgoingNodes(const FGuid& IncomingNodeId) const {
    TArray<SnapLib::ISnapGraphNodePtr> OutgoingNodes;
    for (UGrammarScriptGraphNode* OutgoingNode : GraphNode->OutgoingNodes) {
        if (OutgoingNode->IsA<UGrammarRuleScriptGraphNode>()) {
            OutgoingNodes.Add(MakeShareable(new FSnapGraphGrammarNode(OutgoingNode)));
        }
    }
    return OutgoingNodes;
}

UGrammarScriptGraphNode* FSnapGraphGrammarNode::GetGraphNode() const { return GraphNode.Get(); }


///////////////////////////// FSnapMapGraphModDBItemImpl /////////////////////////////
SnapLib::FModuleNodePtr FSnapMapGraphModDBItemImpl::CreateModuleNode(const FGuid& InNodeId) {
    SnapLib::FModuleNodePtr Node = MakeShareable(new SnapLib::FModuleNode);
    Node->ModuleInstanceId = InNodeId;
    Node->ModuleDBItem = SharedThis(this);

    for (const FSnapMapModuleDatabaseConnectionInfo& DoorInfo : Item.Connections) {
        SnapLib::FModuleDoorPtr Door = MakeShareable(new SnapLib::FModuleDoor);
        Door->ConnectionId = DoorInfo.ConnectionId;
        Door->ConnectionInfo = DoorInfo.ConnectionInfo;
        Door->ConnectionConstraint = DoorInfo.ConnectionConstraint;
        Door->LocalTransform = DoorInfo.Transform;
        Door->Owner = Node;
        Node->Doors.Add(Door);
    }

    return Node;
}

FDAShapeList FSnapMapGraphModDBItemImpl::GetBoundShapes() const {
    if (Item.ModuleBoundShapes.GetTotalCustomShapes() > 0) {
        return Item.ModuleBoundShapes;
    }

    // Add the default bounds as a convex poly
    FDAShapeList ShapeList = Item.ModuleBoundShapes;
    ShapeList.ConvexPolys.Add(FSnapModuleBoundShapeUtils::CreateFallbackModuleConvexHull(Item.ModuleBounds));
    return Item.ModuleBoundShapes;
}

FDungeonCanvasRoomShapeTextureList FSnapMapGraphModDBItemImpl::GetCanvasRoomShapeTextures() {
    return Item.CanvasRoomShapeTextures;
}

TArray<FDungeonPointOfInterest> FSnapMapGraphModDBItemImpl::GetPointsOfInterest() {
    return Item.PointsOfInterest;
}

///////////////////////////// FSnapMapModuleDatabaseImpl /////////////////////////////
FSnapMapModuleDatabaseImpl::FSnapMapModuleDatabaseImpl(USnapMapModuleDatabase* ModuleDB) {
    if (ModuleDB) {
        for (const FSnapMapModuleDatabaseItem& ModuleInfo : ModuleDB->Modules) {
            TArray<SnapLib::IModuleDatabaseItemPtr>& CategoryModuleNames = ModulesByCategory.FindOrAdd(
                ModuleInfo.Category);
            CategoryModuleNames.Add(MakeShareable(new FSnapMapGraphModDBItemImpl(ModuleInfo)));
        }
    }
}

