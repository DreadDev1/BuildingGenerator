//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/FlowEditor/FlowEditor.h"
#include "Core/Editors/FlowEditor/FlowEditorSettings.h"
#include "CellFlowEditor.generated.h"

UCLASS()
class UCellFlowEditorSettings : public UFlowEditorSettings {
	GENERATED_BODY()
public:
	
};


//////////////////// Cell Flow Editor ////////////////////
class FCellFlowEditor : public FFlowEditorBase {
public:
	// FFlowEditorBase
	virtual void InitDungeonConfig(UDungeonConfig* Config) override;
	virtual void CreateDomainEditorsImpl() override;
	virtual FName GetFlowEdAppName() const override;
	virtual ADungeon* CreatePreviewDungeon(UWorld* World) override;
	virtual FText GetEditorBrandingText() const override;
    virtual UFlowEditorSettings* CreateEditorSettingsObject() const override;
	//virtual TSharedRef<SWidget> CreatePerfWidget(const TSharedRef<SDockTab> DockTab, TSharedPtr<SWindow> OwnerWindow) override;
	// End of FFlowEditorBase

	// FAssetEditorToolkit
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	// End of FAssetEditorToolkit
    
	// FGCObject Interface
	virtual FString GetReferencerName() const override;
	// End of FGCObject Interface
	
protected:
	virtual bool RequiresCustomFrameLayout() const override { return true; }
	virtual void  ConfigureDomainObject(IFlowDomainPtr Domain) override;
};

