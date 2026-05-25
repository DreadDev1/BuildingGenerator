//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/ThemeEditor/Visualization/ThemeEditorDungeonActor.h"
#include "Core/Utils/DungeonEditorViewportProperties.h"
#include "Frameworks/ThemeEngine/DungeonThemeAsset.h"

#include "SEditorViewport.h"

class USkyLightComponent;
class USkyAtmosphereComponent;
class FThemeGraphAppMode;
class FDungeonArchitectThemeEditor;
class AThemeEditorMarkerVisualizer;

/**
* Theme Editor Preview viewport widget
*/
class DUNGEONARCHITECTEDITOR_API SThemePreviewViewport
    : public SEditorViewport
    , public FGCObject
    //, public IDungeonEditorViewportPropertiesListener
{
public:
    SLATE_BEGIN_ARGS(SThemePreviewViewport) {}
        SLATE_ARGUMENT(TWeakPtr<FDungeonArchitectThemeEditor>, DungeonEditor)
        SLATE_ARGUMENT(UDungeonThemeAsset*, ObjectToEdit)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    ~SThemePreviewViewport();

    // FGCObject interface
    virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
    virtual FString GetReferencerName() const override;
    // End of FGCObject interface

    /** Set the parent tab of the viewport for determining visibility */
    void SetParentTab(const FName& InAppMode, TSharedRef<SDockTab> InParentTab);

    void RebuildDungeon(const FDungeonBuildSettings& InBuildSettings);

    // SWidget Interface
    virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;
    // End of SWidget Interface

    EVisibility GetToolbarVisibility() const;

    void VisualizeMarkers(const TSet<FName>& InMarkerNames) const;
    void ClearMarkerVisualizations() const;


    /* The nodes that were modified and requires a clean rebuild by the scene provider */
    void SetNodesToRebuild(const TSet<FName>& NodeIds);

    TSharedPtr<class FAdvancedPreviewScene> GetAdvancedPreview() const { return PreviewScene; }

    void ZoomOnNearestNodeMesh(const FName& InNodeId) const;
    FORCEINLINE TObjectPtr<AThemeEditorDungeonActor> GetDungeonActor() const { return Dungeon; }
    void InitializeCamera() const;
    
protected:
    /** SEditorViewport interface */
    virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
    virtual EVisibility OnGetViewportContentVisibility() const override;
    virtual void BindCommands() override;
    virtual void OnFocusViewportToSelection() override;
    virtual TSharedPtr<SWidget> BuildViewportToolbar() override;


    void OnShowPropertyDungeon() const;
    void OnShowPropertySkylight() const;
    void OnShowPropertyDirectionalLight() const;
    void OnShowPropertyAtmosphericFog() const;
    void OnToggleDebugData();

    bool HandleToggleDebugDataIsChecked() const;

    void ShowObjectProperties(UObject* Object, bool bForceRefresh = false) const;
    void CleanupModifiedNodeObjects();
    
private:
    /** Determines the visibility of the viewport. */
    virtual bool IsVisible() const override;

    
private:
    TWeakPtr<FDungeonArchitectThemeEditor> ThemeEditorPtr;
    TObjectPtr<UDungeonThemeAsset> ObjectToEdit = nullptr;

    // The parent tab where this viewport resides, mapped to the app mode id
    TMap<FName, TWeakPtr<SDockTab>> ParentTabs;

    /** Level viewport client */
    TSharedPtr<class FDungeonEditorViewportClient> EditorViewportClient;

    /** The scene for this viewport. */
    TSharedPtr<class FAdvancedPreviewScene> PreviewScene;

    //TSharedPtr<class FDungeonSceneProvider> SceneProvider;

    bool bRequestRebuildPreviewMesh = false;

    bool bDrawDebugData = false;
    bool bForcePreviewRebuildLayout = false;

    TObjectPtr<USkyLightComponent> Skylight = nullptr;
    TObjectPtr<USkyAtmosphereComponent> AtmosphericFog = nullptr;
    TObjectPtr<AThemeEditorDungeonActor> Dungeon = nullptr;

    // The nodes that were modified and requires a clean rebuild by the scene provider
    TSet<FName> NodeObjectsToRebuild;

    FDungeonBuildSettings BuildSettings = {};

    //TObjectPtr<UDungeonBuilder> DungeonBuilder = nullptr;
    //TObjectPtr<UDungeonModel> DungeonModel = nullptr;
    //TObjectPtr<UDungeonQuery> DungeonQuery = nullptr;

    TObjectPtr<AThemeEditorMarkerVisualizer> Visualizer = nullptr;
};

