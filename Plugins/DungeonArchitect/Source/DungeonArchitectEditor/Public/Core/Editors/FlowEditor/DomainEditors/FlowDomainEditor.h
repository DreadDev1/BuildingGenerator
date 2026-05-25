//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Textures/SlateIcon.h"

class FEditorModeTools;
class SWidget;
class IDetailsView;

struct FFlowDomainEditorTabInfo {
    FName TabID;
    FText DisplayName;
    FSlateIcon Icon;
};

typedef TSharedPtr<class FFlowExecNodeState> FFlowExecNodeStatePtr;
typedef TSharedPtr<class IFlowDomain> IFlowDomainPtr;
typedef TWeakPtr<class IFlowDomain> IFlowDomainWeakPtr;

struct FDomainEdInitSettings {
    TSharedPtr<IDetailsView> PropertyEditor;
    TSharedPtr<FEditorModeTools> EditorModeManager;
};

class IFlowDomainEditor {
public:
    virtual ~IFlowDomainEditor() {}
    
    void Initialize(const FDomainEdInitSettings& InSettings);
    IFlowDomainPtr GetDomain() const { return Domain; }
    FName GetDomainID() const;

    // IFlowDomainEditor Interface
    virtual bool IsVisualEditor() const { return true; }
    virtual FFlowDomainEditorTabInfo GetTabInfo() const = 0;
    virtual TSharedRef<SWidget> GetContentWidget() = 0;
    virtual void Build(const FFlowExecNodeStatePtr& State) = 0;
    
    virtual void Tick(float DeltaTime) {}
    virtual void RecenterView(const FFlowExecNodeStatePtr& State) {}
    virtual bool CanSaveThumbnail() const { return false; }
    virtual void SaveThumbnail(const struct FAssetData& InAsset, int32 ThumbSize) {}
    // End IFlowDomainEditor Interface

protected:
    virtual void InitializeImpl(const FDomainEdInitSettings& InSettings) = 0;
    virtual IFlowDomainPtr CreateDomain() const = 0;
    
protected:
    IFlowDomainPtr Domain;
};

