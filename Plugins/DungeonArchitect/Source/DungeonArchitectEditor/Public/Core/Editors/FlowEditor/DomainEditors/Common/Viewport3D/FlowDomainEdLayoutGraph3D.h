//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/FlowEditor/DomainEditors/FlowDomainEditor.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Utils/FlowLayoutGraphVisualization.h"

#include "EngineUtils.h"

class SFlowDomainEdViewport3D;
class USphereComponent;
class UTextRenderComponent;
class AStaticMeshActor;

class FFlowDomainEdLayoutGraph3D
    : public IFlowDomainEditor
    , public FGCObject
    , public TSharedFromThis<FFlowDomainEdLayoutGraph3D>
{
public:
    //~ Begin FFlowDomainEditorBase Interface
    virtual FFlowDomainEditorTabInfo GetTabInfo() const override;
    virtual TSharedRef<SWidget> GetContentWidget() override;
    virtual void Build(const FFlowExecNodeStatePtr& State) override;
    virtual void RecenterView(const FFlowExecNodeStatePtr& State) override;
    virtual void Tick(float DeltaTime) override;
    //~ End FFlowDomainEditorBase Interface

    //~ Begin FGCObject Interface
    virtual void AddReferencedObjects( FReferenceCollector& Collector ) override;
    virtual FString GetReferencerName() const override;
    //~ End FGCObject Interface

protected:
    template<typename T>
        T* FindActor(const UWorld* InWorld) {
        for (TActorIterator<T> It(InWorld); It; ++It) {
            return *It;
        }
        return nullptr;
    }
    virtual void InitializeImpl(const FDomainEdInitSettings& InSettings) override;
    
private:
    virtual void BuildLayoutGraphVisualization(AFlowLayoutGraphVisualizer* LayoutGraphVisualizer, const FFlowExecNodeStatePtr& State);
    virtual void BuildCustomVisualization(UWorld* InWorld, const FFlowExecNodeStatePtr& State);
    virtual TSharedPtr<SFlowDomainEdViewport3D> CreateViewport() const;
    virtual float GetLayoutGraphVisualizerZOffset();

    void OnActorSelectionChanged(AActor* InActor);
    void OnActorDoubleClicked(AActor* InActor);

protected:
    TSharedPtr<SFlowDomainEdViewport3D> Viewport;
    TSharedPtr<SWidget> ContentWidget;
    TWeakObjectPtr<UFDAbstractNodePreview> SelectedNode;
    TObjectPtr<AStaticMeshActor> Skybox = nullptr;

protected:
    FDAAbstractGraphVisualizerSettings VisualizerSettings;
};

