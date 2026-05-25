//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/CanvasEditor/DungeonCanvasEditorSettings.h"
#include "Core/Utils/DungeonEditorViewportProperties.h"
#include "Frameworks/Canvas/DungeonCanvas.h"
#include "Frameworks/Canvas/DungeonCanvasMaterialLayer.h"
#include "Frameworks/GraphGrammar/Editor/SEditableListView.h"

#include "BlueprintEditor.h"

class UDungeonCanvasMaterialLayer;
class UDungeonCanvasMaterialTheme;
class ADungeon;
class SDungeonCanvasViewport;
class FDungeonCanvasBlueprintEditorToolbar;
class UDungeonCanvasBlueprint;

class DUNGEONARCHITECTEDITOR_API FDungeonCanvasMaterialThemeEditor
	: public FAssetEditorToolkit
	, public FTickableEditorObject
	, public FGCObject
	, public FNotifyHook
	, public IDungeonEditorViewportPropertiesListener
{
	typedef FAssetEditorToolkit Super;
public:
	FDungeonCanvasMaterialThemeEditor();
	void InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UDungeonCanvasMaterialTheme* InThemeAsset);

	virtual void Compile();

	//~ Begin IToolkit Interface
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetDocumentationLink() const override;
	virtual FText GetToolkitToolTipText() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void SaveAsset_Execute() override;
	//~ End IToolkit Interface

	//~ Begin FTickableEditorObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	//~ End FTickableEditorObject Interface

	//~ Begin IDungeonEditorViewportPropertiesListener Interface
	virtual void OnPropertyChanged(FString PropertyName, class UDungeonEditorViewportProperties* Properties) override;
	//~ End IDungeonEditorViewportPropertiesListener Interface
	
	virtual TSharedPtr<FDungeonCanvasBlueprintEditorToolbar> GetDungeonCanvasToolbarBuilder() { return DungeonCanvasToolbar; }

	UDungeonCanvasComponent* GetInstance() const;
	TSharedPtr<SDungeonCanvasViewport> GetCanvasViewport() const { return CanvasViewport; }
	TSharedPtr<SWidget> GetMaterialLayerListWidget() const { return MaterialLayerList; }
	void RecreateDungeonBuilder(UDungeonEditorViewportProperties* InViewportProperties);

	ESimpleElementBlendMode GetColourChannelBlendMode() const;

	EDungeonCanvasEditorSampling GetSampling() const;
	
private:
	const TArray<TObjectPtr<UDungeonCanvasMaterialLayer>>* GetMaterialLayerList() const;
	void OnMaterialLayerSelectionChanged(TObjectPtr<UDungeonCanvasMaterialLayer> Item, ESelectInfo::Type SelectInfo);
	void AddNewMaterialLayer(const FText& InLayerName, const FText& InLayerDescription, const TSoftObjectPtr<UMaterialFunctionMaterialLayer>& InMaterialLayer, const TSoftObjectPtr<UMaterialFunctionMaterialLayerBlend>& InMaterialBlend);
	void OnMaterialLayerDelete(TObjectPtr<UDungeonCanvasMaterialLayer> Item);
	void OnMaterialLayerReordered(TObjectPtr<UDungeonCanvasMaterialLayer> Source, TObjectPtr<UDungeonCanvasMaterialLayer> Dest);
	FText GetMaterialLayerListRowText(TObjectPtr<UDungeonCanvasMaterialLayer> Item) const;
	FText GetLayerListRowDescriptionText(TObjectPtr<UDungeonCanvasMaterialLayer> Item) const;
	EVisibility IsLayerListRowDescriptionVisible(TObjectPtr<UDungeonCanvasMaterialLayer> Item) const;
	TSharedPtr<SWidget> CreateMaterialLayerListItemWidget(TObjectPtr<UDungeonCanvasMaterialLayer> InMaterialLayer);

	void UpdateAssetThumbnail() const;
	
	TSharedRef<SDockTab> SpawnTab_MaterialLayers(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> SpawnTab_Preview3D(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& SpawnTabArgs);
	
	TSharedRef<SWidget> GetAddMaterialLayerMenu();
	void RandomizePreviewDungeon();

protected:
	//~ Begin FGCObject Interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;
	//~ End FGCObject Interface

	//~ Begin FNotifyHook Interface
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;
	virtual void NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, class FEditPropertyChain* PropertyThatChanged ) override;
	//~ End FNotifyHook Interface

protected:
	/** Binds the FDungeonCanvasEditorCommands commands to functions in this editor. */
	void BindCommands();

	void CreateDefaultCommands();
	
	/** Undo the last action. */
	void UndoAction();

	/** Redo the last action that was undone. */
	void RedoAction();

	void EditPreviewDungeonSettings_Clicked();
	void EditThemeSettings_Clicked() const;
	void CompileTheme_Clicked();

	bool IsDetailsPanelEditingDungeonPreviewSettings() const;
	bool IsDetailsPanelEditingCanvasThemeSettings() const;

private:
	/** Extends the menu. */
	void ExtendMenu();

	/** Extends the toolbar. */
	void ExtendToolbar();

	/** Fills the toolbar with content. */
	void FillToolbar(FToolBarBuilder& ToolbarBuilder);

	/** Makes a newly compiled/opened dungeon canvas instance visible in the editor. */
	void UpdateInstance();

	void BuildPreviewDungeon();
	//void RefreshMaterialListView() const;
	
	void ShowPreviewDungeonSettings();

	void ShowDetailsForSingleObject(UObject* Object, bool bForceRefresh = false) const;
	
	TSharedRef<SWidget> GeneratePreviewBuilderClassDropDown();
	
private:
	/** The toolbar builder that is used to customize the toolbar of this editor. */
	TSharedPtr<FDungeonCanvasBlueprintEditorToolbar> DungeonCanvasToolbar;

protected:
	/** The extender to pass to the level editor to extend it's window menu. */
	TSharedPtr<FExtender> MenuExtender;

	/** The toolbar extender of this editor. */
	TSharedPtr<FExtender> ToolbarExtender;

	TSharedPtr<IDetailsView> DetailsPanel;
	
	/** The dungeon canvas actor */
	TWeakObjectPtr<UDungeonCanvasComponent> CanvasInstance{};
	TWeakObjectPtr<ADungeon> PreviewDungeon;

private:
    TObjectPtr<UDungeonCanvasMaterialTheme> AssetBeingEdited = nullptr;

	/** Viewport */
	TSharedPtr<SDungeonCanvasViewport> CanvasViewport;

	TWeakObjectPtr<UDungeonCanvasMaterialLayer> ActiveMaterialLayer;
	TWeakObjectPtr<UDungeonCanvasEditorDefaults> EditorDefaultSettings;

	/* Material Layer List */
	TSharedPtr<SEditableListView<TObjectPtr<UDungeonCanvasMaterialLayer>>> MaterialLayerList;
	
	TUniquePtr<FPreviewScene> PreviewScene;
	
	/** This toolkit's current sampling mode **/
	EDungeonCanvasEditorSampling Sampling;
	bool bRecompileNextFrame{};
};

