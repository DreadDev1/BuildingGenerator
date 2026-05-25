//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/FlowEditor/DomainEditors/FlowDomainEditor.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractItem.h"

#include "UObject/GCObject.h"

class UFlowLayoutEdGraph;
class UFlowLayoutEdGraphNode;
class UEdGraphNode;
class SGraphEditor;

class FFlowDomainEdLayoutGraph
    : public IFlowDomainEditor
    , public FGCObject
{
public:
    //~ Begin FFlowDomainEditorBase Interface
    virtual FFlowDomainEditorTabInfo GetTabInfo() const override;
    virtual TSharedRef<SWidget> GetContentWidget() override;
    virtual void Tick(float DeltaTime) override;
    virtual void Build(const FFlowExecNodeStatePtr& State) override;
    virtual void RecenterView(const FFlowExecNodeStatePtr& State) override;
    //~ End FFlowDomainEditorBase Interface

    //~ Begin FGCObject Interface
    virtual void AddReferencedObjects( FReferenceCollector& Collector ) override;
    virtual FString GetReferencerName() const override;
    //~ End FGCObject Interface
    
    void SelectItem(const FGuid& InItemId) const;
    static void GetAllItems(FFlowExecNodeStatePtr State, TArray<UFlowGraphItem*>& OutItems);

    void ClearAllSelections() const;
    void SelectNode(const FVector& InNodeCoord, bool bSelected) const;
    
    class IMediator {
    public:
        virtual ~IMediator() {}
        virtual void OnAbstractNodeSelectionChanged(TArray<UFlowLayoutEdGraphNode*> EdNodes) = 0;
        virtual void OnAbstractItemWidgetClicked(const FGuid& InItemId, bool bDoubleClicked) = 0;
    };

    FORCEINLINE void SetMediator(TSharedPtr<IMediator> InMediator) { Mediator = InMediator; }

private:
    virtual UFlowLayoutEdGraph* CreateGraph() const = 0;
    virtual UFlowLayoutEdGraphNode* CreateGraphNode() const = 0;
    
    virtual void InitializeImpl(const FDomainEdInitSettings& InSettings) override;
    
    void OnItemWidgetClicked(const FGuid& InItemId, bool bDoubleClicked) const;
    void OnAbstractNodeSelectionChanged(const TSet<class UObject*>& InSelectedObjects) const;
    
protected:
    TObjectPtr<UFlowLayoutEdGraph> AbstractGraph = nullptr;
    TSharedPtr<SGraphEditor> AbstractGraphEditor;
    TSharedPtr<class FFlowLayoutGraphHandler> AbstractGraphHandler;

    TWeakPtr<IMediator> Mediator;
};

