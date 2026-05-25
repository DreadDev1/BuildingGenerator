//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/FlowEditor/DomainEditors/FlowDomainEditor.h"
#include "Frameworks/ThemeEngine/SceneProviders/DungeonSceneCollector.h"

#include "EngineUtils.h"

class FPreviewScene;
class FFlowDomainEdViewportClient2D;
class UFDAbstractNodePreview;
class SFlowDomainEdViewport2D;

class FFlowDomainEdLayoutGraph2D
	: public IFlowDomainEditor
	, public FGCObject
	, public TSharedFromThis<FFlowDomainEdLayoutGraph2D>
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

private:
	virtual void InitializeImpl(const FDomainEdInitSettings& InSettings) override;
	virtual TSharedPtr<FFlowDomainEdViewportClient2D> CreateViewportClient(const FDomainEdInitSettings& InSettings) const = 0;
	
protected:
	TUniquePtr<FPreviewScene> PreviewScene;
	TSharedPtr<SFlowDomainEdViewport2D> Viewport;
	TSharedPtr<SWidget> ContentWidget;
	TWeakObjectPtr<UFDAbstractNodePreview> SelectedNode;
};

