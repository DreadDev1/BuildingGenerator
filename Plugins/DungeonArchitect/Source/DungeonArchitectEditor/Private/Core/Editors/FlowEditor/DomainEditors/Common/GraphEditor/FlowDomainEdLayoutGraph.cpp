//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/FlowEditor/DomainEditors/Common/GraphEditor/FlowDomainEdLayoutGraph.h"

#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraph.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractLink.h"
#include "Frameworks/Flow/Domains/LayoutGraph/FlowLayoutEdGraph.h"
#include "Frameworks/Flow/Domains/LayoutGraph/FlowLayoutEdGraphNode.h"
#include "Frameworks/Flow/Domains/LayoutGraph/FlowLayoutGraphHandler.h"
#include "Frameworks/Flow/ExecGraph/FlowExecTask.h"

#include "EdGraph/EdGraphPin.h"
#include "GraphEditor.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "FlowDomainEdAbstractGraph"
DEFINE_LOG_CATEGORY_STATIC(LogDomainEdAbstractGraph, Log, All);

void FFlowDomainEdLayoutGraph::InitializeImpl(const FDomainEdInitSettings& InSettings) {
    AbstractGraph = CreateGraph();
    AbstractGraph->OnItemWidgetClicked.BindRaw(this, &FFlowDomainEdLayoutGraph::OnItemWidgetClicked);

    // Create the appearance info
    FGraphAppearanceInfo AppearanceInfo;
    AppearanceInfo.CornerText = LOCTEXT("FlowAbstractGraphBranding", "Layout Graph");
    AbstractGraphHandler = MakeShareable(new FFlowLayoutGraphHandler);
    AbstractGraphHandler->Bind();
    AbstractGraphHandler->SetPropertyEditor(InSettings.PropertyEditor);

    AbstractGraphEditor = SNew(SGraphEditor)
        .AdditionalCommands(AbstractGraphHandler->GraphEditorCommands)
        .Appearance(AppearanceInfo)
        .GraphToEdit(AbstractGraph)
        .IsEditable(true)
        .ShowGraphStateOverlay(false)
        .GraphEvents(AbstractGraphHandler->GraphEvents);

    AbstractGraphHandler->SetGraphEditor(AbstractGraphEditor);
    AbstractGraphHandler->OnNodeSelectionChanged.BindRaw(this, &FFlowDomainEdLayoutGraph::OnAbstractNodeSelectionChanged);
}

FFlowDomainEditorTabInfo FFlowDomainEdLayoutGraph::GetTabInfo() const {
    static const FFlowDomainEditorTabInfo TabInfo = {
        TEXT("AbstractGraphTab"),
        LOCTEXT("LayoutGraphTabLabel", "Layout Graph"),
        FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details")
    };

    return TabInfo;
}

TSharedRef<SWidget> FFlowDomainEdLayoutGraph::GetContentWidget() {
    return AbstractGraphEditor.ToSharedRef();
}

void FFlowDomainEdLayoutGraph::Tick(float DeltaTime) {
}

void FFlowDomainEdLayoutGraph::Build(const FFlowExecNodeStatePtr& State) {
    if (!State.IsValid()) return;
    
    // Clear out the existing abstract graph
    {
        TArray<UEdGraphNode*> OldNodes = AbstractGraph->Nodes;
        for (UEdGraphNode* Node : OldNodes) {
            AbstractGraph->RemoveNode(Node);
        }
    }

    UFlowAbstractGraphBase* ScriptGraph = State->GetState<UFlowAbstractGraphBase>(UFlowAbstractGraphBase::StateTypeID);
    if (!ScriptGraph) {
        return;
    }

    AbstractGraph->ScriptGraph = ScriptGraph;

    TMap<FGuid, UFlowLayoutEdGraphNode*> EdNodes;
    for (UFlowAbstractNode* ScriptNode : ScriptGraph->GraphNodes) {
        if (!ScriptNode) {
            continue;
        }
        
        UFlowLayoutEdGraphNode* EdNode = CreateGraphNode();

        constexpr float NodeSeparation = 120.0f;
        EdNode->ScriptNode = ScriptNode;
        EdNode->NodeGuid = ScriptNode->NodeId;
        EdNode->NodePosX = ScriptNode->Coord.X * NodeSeparation;
        EdNode->NodePosY = ScriptNode->Coord.Y * NodeSeparation;

        EdNodes.Add(EdNode->NodeGuid, EdNode);
    }

    // Create the links
    for (const UFlowAbstractLink* ScriptLink : ScriptGraph->GraphLinks) {
        UFlowLayoutEdGraphNode** SourcePtr = EdNodes.Find(ScriptLink->Source);
        UFlowLayoutEdGraphNode** DestPtr = EdNodes.Find(ScriptLink->Destination);
        if (!SourcePtr || !DestPtr) {
            UE_LOG(LogDomainEdAbstractGraph, Warning, TEXT("Failed to create link in abstract graph. Invalid state"));
            continue;
        }

        UFlowLayoutEdGraphNode* Source = *SourcePtr;
        UFlowLayoutEdGraphNode* Dest = *DestPtr;
        if (!Source || !Dest) continue;

        Source->GetOutputPin()->MakeLinkTo(Dest->GetInputPin());
    }

    AbstractGraph->NotifyGraphChanged();
}

void FFlowDomainEdLayoutGraph::SelectItem(const FGuid& InItemId) const {
    AbstractGraph->SelectedItemId = InItemId;
}

void FFlowDomainEdLayoutGraph::GetAllItems(FFlowExecNodeStatePtr State, TArray<UFlowGraphItem*>& OutItems) {
    if (State.IsValid()) {
        UFlowAbstractGraphBase* AbstractScriptGraph = State->GetState<UFlowAbstractGraphBase>(UFlowAbstractGraphBase::StateTypeID);
        AbstractScriptGraph->GetAllItems(OutItems);
    }
}

void FFlowDomainEdLayoutGraph::ClearAllSelections() const {
    AbstractGraphEditor->ClearSelectionSet();
}

void FFlowDomainEdLayoutGraph::SelectNode(const FVector& InNodeCoord, bool bSelected) const {
    UFlowLayoutEdGraphNode* TargetNode = nullptr;
    for (UEdGraphNode* EdNode : AbstractGraph->Nodes) {
        if (UFlowLayoutEdGraphNode* AbstractNode = Cast<UFlowLayoutEdGraphNode>(EdNode)) {
            FVector NodeCoord = AbstractNode->ScriptNode->Coord;
            if (NodeCoord.Equals(InNodeCoord)) {
                TargetNode = AbstractNode;
            }
        }
    }

    if (TargetNode) {
        AbstractGraphEditor->SetNodeSelection(TargetNode, bSelected);
    }
}

void FFlowDomainEdLayoutGraph::OnItemWidgetClicked(const FGuid& InItemId, bool bDoubleClicked) const {
    SelectItem(InItemId);
    
    TSharedPtr<IMediator> MediatorPtr = Mediator.IsValid() ? Mediator.Pin() : nullptr;
    if (MediatorPtr.IsValid()) {
        MediatorPtr->OnAbstractItemWidgetClicked(InItemId, bDoubleClicked);
    }
}

void FFlowDomainEdLayoutGraph::OnAbstractNodeSelectionChanged(const TSet<UObject*>& InSelectedObjects) const {
    TArray<UFlowLayoutEdGraphNode*> EdNodes;
    for (UObject* SelectedObject : InSelectedObjects) {
        if (UFlowLayoutEdGraphNode* EdNode = Cast<UFlowLayoutEdGraphNode>(SelectedObject)) {
            EdNodes.Add(EdNode);
        }
    }

    const TSharedPtr<IMediator> MediatorPtr = Mediator.IsValid() ? Mediator.Pin() : nullptr;
    if (MediatorPtr.IsValid()) {
        MediatorPtr->OnAbstractNodeSelectionChanged(EdNodes);
    }
}

void FFlowDomainEdLayoutGraph::RecenterView(const FFlowExecNodeStatePtr& State) {
    if (AbstractGraphEditor.IsValid()) {
        AbstractGraphEditor->ZoomToFit(false);
    }
}

void FFlowDomainEdLayoutGraph::AddReferencedObjects(FReferenceCollector& Collector) {
    if (AbstractGraph) {
        Collector.AddReferencedObject(AbstractGraph);
    }
}

FString FFlowDomainEdLayoutGraph::GetReferencerName() const {
    static const FString NameString = TEXT("FFlowDomainEdAbstractGraph");
    return NameString;
}


#undef LOCTEXT_NAMESPACE

