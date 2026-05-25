//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "SEditorViewport.h"

class FComponentVisualizer;
DECLARE_DELEGATE_OneParam(FFDViewportActorMouseEvent, AActor*);

// Flow Ed ViewportBase Widget
class DUNGEONARCHITECTEDITOR_API SFlowDomainEdViewport3D : public SEditorViewport, public FGCObject {
public:
    SLATE_BEGIN_ARGS(SFlowDomainEdViewport3D) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual ~SFlowDomainEdViewport3D() override;

    // FGCObject interface
    virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
    virtual FString GetReferencerName() const override;
    // End of FGCObject interface

    /** Set the parent tab of the viewport for determining visibility */
    void SetParentTab(TSharedRef<SDockTab> InParentTab) { ParentTab = InParentTab; }
    
    virtual UWorld* GetWorld() const override;
    TSharedPtr<FPreviewScene> GetPreviewScene() const { return PreviewScene; }
    void RegisterComponentForVisualization(const TSharedPtr<FComponentVisualizer>& InComponentVisualizer, TObjectPtr<UActorComponent> InComponent) const;
    void ClearComponentVisualizers() const;

    FFDViewportActorMouseEvent& GetActorSelectionChanged();
    FFDViewportActorMouseEvent& GetActorDoubleClicked();

protected:
    /** SEditorViewport interface */
    virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
    virtual EVisibility OnGetViewportContentVisibility() const override { return EVisibility::Visible; }

private:
    /** Determines the visibility of the viewport. */
    virtual bool IsVisible() const override;

protected:
    /** The parent tab where this viewport resides */
    TWeakPtr<SDockTab> ParentTab;

    /** Level viewport client */
    TSharedPtr<class FFlowDomainEdViewportClient3D> EditorViewportClient;

    /** The scene for this viewport. */
    TSharedPtr<FPreviewScene> PreviewScene;
};

