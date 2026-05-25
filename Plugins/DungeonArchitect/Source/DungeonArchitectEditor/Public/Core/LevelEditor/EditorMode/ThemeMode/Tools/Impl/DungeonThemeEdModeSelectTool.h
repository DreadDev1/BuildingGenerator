//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/Tools/DungeonThemeEdModeToolBase.h"
#include "DungeonThemeEdModeSelectTool.generated.h"

class SDADraggableToolOverlayWidget;
class UDungeonScenePostProcess;
class FDAAssetThumbnailCacheManager;
class SBox;

UCLASS()
class UDungeonThemeEdModeSelectTool : public UDungeonThemeEdModeToolBase
{
	GENERATED_BODY()

public:
	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	
	virtual bool IsActorSelectionAllowed(AActor* Actor, bool bInSelection) override;
	virtual void OnSelectedActorChanged() override;
	virtual void OnSelectedNodesChanged(const TSet<UObject*>& SelectedNodes) override;
	
	virtual void OnActorDoubleClicked(AActor* InActor) override;
	virtual void OnThemeNodeDoubleClicked(UEdGraphNode_DungeonThemeBase* InThemeNode) override;

	virtual void AddOverlayWidgets(const TSharedRef<const FModeToolkit>& InToolkit, TSharedPtr<IAssetViewport> InViewport = nullptr) override;
	virtual void RemoveOverlayWidgets(const TSharedRef<const FModeToolkit>& InToolkit, TSharedPtr<IAssetViewport> InViewport = nullptr) override;
	
protected:
	virtual UMaterialInterface* GetPostProcessMaterialTemplate() const override;
	
private:
	FText GetOverlayPrimaryMessage() const;
	FText GetOverlaySecondaryMessage() const;
	void UpdateNodeThumbnailWidget();
	
public:
	static const FString ToolIdentifier;
	TSharedPtr<SDADraggableToolOverlayWidget> ToolOverlayWidget;
	TSharedPtr<SBox> ThumbnailWidgetHost;
	TSharedPtr<FDAAssetThumbnailCacheManager> ThumbnailCacheManager;
};

