//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "DungeonArchitectEditorModule.h"

#include "Core/Common/DungeonArchitectCommands.h"
#include "Core/Common/Utils/DungeonEditorService.h"
#include "Core/Dungeon.h"
#include "Core/Editors/CanvasEditor/DungeonCanvasDetailCustomization.h"
#include "Core/Editors/CanvasEditor/DungeonCanvasEditorCommands.h"
#include "Core/Editors/CanvasEditor/DungeonCanvasEditorSettings.h"
#include "Core/Editors/FlowEditor/DomainEditors/Impl/CellFlow/Viewport3D/CellFlowDomainEdViewportCommands.h"
#include "Core/Editors/FlowEditor/FlowEditorCommands.h"
#include "Core/Editors/FlowEditor/FlowEditorCustomizations.h"
#include "Core/Editors/FlowEditor/Panels/Viewport/SFlowPreview3DViewportToolbar.h"
#include "Core/Editors/LaunchPad/LaunchPad.h"
#include "Core/Editors/LaunchPad/Styles/LaunchPadStyle.h"
#include "Core/Editors/SnapConnectionEditor/Preview3D/SSnapConnectionPreview3DViewportToolbar.h"
#include "Core/Editors/SnapConnectionEditor/SnapConnectionEditorCommands.h"
#include "Core/Editors/SnapConnectionEditor/SnapConnectionEditorCustomization.h"
#include "Core/Editors/SnapMapEditor/Viewport/SSnapMapEditorViewportToolbar.h"
#include "Core/Editors/ThemeEditor/AppModes/MarkerGenerator/MarkerGeneratorAppModeCommands.h"
#include "Core/Editors/ThemeEditor/AppModes/MarkerGenerator/PatternEditor/PatternEditorMode.h"
#include "Core/Editors/ThemeEditor/AppModes/MarkerGenerator/PatternGraph/Editor/PatternGraphPinFactory.h"
#include "Core/Editors/ThemeEditor/AppModes/ThemeGraph/ThemeGraphAppMode.h"
#include "Core/Editors/ThemeEditor/DungeonArchitectThemeEditor.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorBase.h"
#include "Core/Editors/ThemeEditor/Widgets/GraphPanelNodeFactory_DungeonTheme.h"
#include "Core/Editors/ThemeEditor/Widgets/SDungeonEditorViewportToolbar.h"
#include "Core/LevelEditor/Assets/Canvas/Themes/UDungeonCanvasMaterialThemeAssetTypeAction.h"
#include "Core/LevelEditor/Assets/CellFlow/CellFlowAssetTypeAction.h"
#include "Core/LevelEditor/Assets/Forge/DungeonForgeAssetTypeAction.h"
#include "Core/LevelEditor/Assets/GridFlow/GridFlowAssetTypeActions.h"
#include "Core/LevelEditor/Assets/PlaceableMarker/PlaceableMarkerAssetBroker.h"
#include "Core/LevelEditor/Assets/PlaceableMarker/PlaceableMarkerAssetTypeActions.h"
#include "Core/LevelEditor/Assets/SnapConnection/SnapConnectionAssetBroker.h"
#include "Core/LevelEditor/Assets/SnapConnection/SnapConnectionAssetTypeActions.h"
#include "Core/LevelEditor/Assets/SnapGridFlow/SnapGridFlowAssetTypeActions.h"
#include "Core/LevelEditor/Assets/SnapMap/ModuleDatabase/SnapMapModuleDBTypeActions.h"
#include "Core/LevelEditor/Assets/SnapMap/SnapMapAssetTypeActions.h"
#include "Core/LevelEditor/Assets/Theme/DungeonThemeAssetTypeActions.h"
#include "Core/LevelEditor/Config/CustomInputMapping.h"
#include "Core/LevelEditor/Config/CustomInputMappingDetails.h"
#include "Core/LevelEditor/Customizations/DungeonActorEditorCustomization.h"
#include "Core/LevelEditor/Customizations/DungeonArchitectContentBrowserExtensions.h"
#include "Core/LevelEditor/Customizations/DungeonArchitectGraphNodeCustomization.h"
#include "Core/LevelEditor/Customizations/DungeonArchitectStyle.h"
#include "Core/LevelEditor/Customizations/DungeonForgeEditorCustomization.h"
#include "Core/LevelEditor/Customizations/SnapMapModuleBoundsCustomization.h"
#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorCommands.h"
#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorModeCommands.h"
#include "Core/LevelEditor/EditorMode/Legacy/DungeonEdMode/DungeonEdModeHandlerFactory.h"
#include "Core/LevelEditor/EditorMode/Legacy/DungeonLegacyEdMode.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdModeCommands.h"
#include "Core/LevelEditor/EditorMode/ToolsMode/DungeonToolsEditorCommands.h"
#include "Core/LevelEditor/EditorMode/ToolsMode/DungeonToolsEditorModeStyles.h"
#include "Core/LevelEditor/Extenders/EditorUIExtender.h"
#include "Core/LevelEditor/Placements/DungeonArchitectPlacements.h"
#include "Core/LevelEditor/Visualizers/PlaceableMarkerVisualizer.h"
#include "Core/LevelEditor/Visualizers/SceneDebugDataComponentVisualizer.h"
#include "Core/Volumes/DungeonVolume.h"
#include "Frameworks/Canvas/DungeonCanvasMaterialLayer.h"
#include "Frameworks/Flow/Domains/LayoutGraph/FlowLayoutGraphPanelNodeFactory.h"
#include "Frameworks/Flow/Domains/Tilemap/Graph/TilemapGraphInfrastructure.h"
#include "Frameworks/Flow/ExecGraph/FlowExecGraphPanelNodeFactory.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutVisualization.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"
#include "Frameworks/Forge/Graph/DungeonForgeEditorGraphNodeFactory.h"
#include "Frameworks/GraphGrammar/ExecutionGraph/EdGraphSchema_GrammarExec.h"
#include "Frameworks/GraphGrammar/ExecutionGraph/GrammarExecGraphConnectionDrawingPolicy.h"
#include "Frameworks/GraphGrammar/ExecutionGraph/GraphPanelNodeFactory_GrammarExec.h"
#include "Frameworks/GraphGrammar/ExecutionGraph/Nodes/EdGraphNode_GrammarExecRuleNode.h"
#include "Frameworks/GraphGrammar/RuleGraph/EdGraphSchema_Grammar.h"
#include "Frameworks/GraphGrammar/RuleGraph/GrammarGraphConnectionDrawingPolicy.h"
#include "Frameworks/GraphGrammar/RuleGraph/GraphPanelNodeFactory_Grammar.h"
#include "Frameworks/MarkerGenerator/Impl/Grid/MarkerGenGridLayer.h"
#include "Frameworks/Snap/Lib/Connection/SnapConnectionActor.h"
#include "Frameworks/Snap/Lib/Connection/SnapConnectionComponent.h"
#include "Frameworks/Snap/Lib/Module/SnapModuleBoundShape.h"
#include "Frameworks/Snap/SnapGridFlow/SnapGridFlowModuleDatabase.h"
#include "Frameworks/Snap/SnapMap/SnapMapModuleDatabase.h"
#include "Frameworks/TestRunner/DATestRunnerCommands.h"
#include "Frameworks/ThemeEngine/Markers/PlaceableMarker.h"

#include "AssetToolsModule.h"
#include "ComponentAssetBroker.h"
#include "ComponentVisualizer.h"
#include "Editor/UnrealEdEngine.h"
#include "IAssetTools.h"
#include "ISettingsModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "UnrealEdGlobals.h"

#define LOCTEXT_NAMESPACE "DungeonArchitectEditorModule"


class FDungeonArchitectEditorModule : public IDungeonArchitectEditorModule {
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override {
		FCoreDelegates::OnPostEngineInit.AddRaw(this, &FDungeonArchitectEditorModule::OnPostEngineInit);
        
        FDungeonEditorThumbnailPool::Create();
        InitializeStyles();
        RegisterCommands();

        UIExtender.Extend();

        // Add a category for the dungeon architect assets
        {
            IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
            DungeonAssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Dungeon Architect")), LOCTEXT("DungeonArchitectAssetCategory", "Dungeon Architect"));
        }

        // Register the details customization
        {
            FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		    RegisterCustomClassLayout<ADungeon, FDungeonActorEditorCustomization>(PropertyEditorModule);
		    RegisterCustomClassLayout<ADungeonVolume, FDungeonArchitectVolumeCustomization>(PropertyEditorModule);
		    RegisterCustomClassLayout<UEdGraphNode_DungeonThemeActorBase, FDungeonArchitectVisualGraphNodeCustomization>(PropertyEditorModule);
		    RegisterCustomClassLayout<UEdGraphNode_GrammarExecRuleNode, FDAExecRuleNodeCustomization>(PropertyEditorModule);
		    RegisterCustomClassLayout<UDungeonEditorViewportProperties, FDungeonEditorViewportPropertiesCustomization>(PropertyEditorModule);
		    RegisterCustomClassLayout<ASnapConnectionActor, FSnapConnectionActorCustomization>(PropertyEditorModule);
		    RegisterCustomClassLayout<USnapMapModuleDatabase, FSnapModuleDatabaseCustomization>(PropertyEditorModule);
		    RegisterCustomClassLayout<USnapGridFlowModuleDatabase, FSnapGridFlowModuleDatabaseCustomization>(PropertyEditorModule);
		    RegisterCustomClassLayout<ADACustomInputConfigBinder, FDACustomInputBinderCustomization>(PropertyEditorModule);
		    RegisterCustomClassLayout<UMarkerGenGridLayer, FMGPatternGridLayerCustomization>(PropertyEditorModule);
		    RegisterCustomClassLayout<ASnapModuleBoundShape, FSnapMapModuleBoundsCustomization>(PropertyEditorModule);
		    RegisterCustomClassLayout<UDungeonCanvasMaterialLayer, FDungeonCanvasMaterialLayerCustomization>(PropertyEditorModule);

		    // Dungeon Forge Graph Node Customizations
		    RegisterCustomClassLayout<UDungeonForgeNodeSettings, FDungeonForgeEditorNodeCustomization>(PropertyEditorModule);

		    // Flow Task Node Customizations
		    FFlowEditorTaskCustomizations::RegisterTaskNodes(PropertyEditorModule);

            PropertyEditorModule.NotifyCustomizationModuleChanged();
        }

        // Register the editor mode handlers for the dungeon builders
        FDungeonEdModeHandlerFactory::Register();

        // Add content browser hooks
        FDungeonArchitectContentBrowserExtensions::InstallHooks();

        // Register asset types
        IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
        RegisterAssetTypeAction(AssetTools, MakeShareable(new FDungeonThemeAssetTypeActions));
        RegisterAssetTypeAction(AssetTools, MakeShareable(new FSnapMapAssetTypeActions));
        RegisterAssetTypeAction(AssetTools, MakeShareable(new FGridFlowFlowAssetTypeActions));
        RegisterAssetTypeAction(AssetTools, MakeShareable(new FSnapConnectionAssetTypeActions));
        RegisterAssetTypeAction(AssetTools, MakeShareable(new FSnapMapModuleDBTypeActions));
        RegisterAssetTypeAction(AssetTools, MakeShareable(new FSnapGridFlowAssetTypeActions));
        RegisterAssetTypeAction(AssetTools, MakeShareable(new FCellFlowAssetTypeActions));
        RegisterAssetTypeAction(AssetTools, MakeShareable(new FCellFlowConfigMarkerSettingsTypeActions));
        RegisterAssetTypeAction(AssetTools, MakeShareable(new FSnapGridFlowModuleBoundsAssetTypeActions));
        RegisterAssetTypeAction(AssetTools, MakeShareable(new FSnapGridFlowModuleDatabaseTypeActions));
        RegisterAssetTypeAction(AssetTools, MakeShareable(new FPlaceableMarkerAssetTypeActions));
        RegisterAssetTypeAction(AssetTools, MakeShareable(new FDungeonCanvasMaterialThemeAssetTypeAction));
        RegisterAssetTypeAction(AssetTools, MakeShareable(new FDungeonForgeAssetTypeActions));
        //RegisterAssetTypeAction(AssetTools, MakeShareable(new FDungeonCanvasMaterialThemeBlueprintDeprecatedTypeAction));

        // Register custom graph nodes
        RegisterVisualNodeFactory(MakeShareable(new FGraphPanelNodeFactory_DungeonTheme));
        RegisterVisualNodeFactory(MakeShareable(new FGraphPanelNodeFactory_Grammar));
        RegisterVisualNodeFactory(MakeShareable(new FGraphPanelNodeFactory_GrammarExec));
        RegisterVisualNodeFactory(MakeShareable(new FFlowExecGraphPanelNodeFactory));
        RegisterVisualNodeFactory(MakeShareable(new FFlowLayoutGraphPanelNodeFactory));
        RegisterVisualNodeFactory(MakeShareable(new FFlowTilemapGraphPanelNodeFactory));
        RegisterVisualNodeFactory(MakeShareable(new FDungeonForgeEditorGraphNodeFactory));

        // Register custom graph pins
        RegisterVisualPinFactory(MakeShareable(new FMGPatternGraphPinFactory));

        // Register the asset brokers (used for asset to component mapping) 
        RegisterAssetBroker<FSnapConnectionAssetBroker, USnapConnectionComponent>(true, true);
        RegisterAssetBroker<FPlaceableMarkerAssetBroker, UPlaceableMarkerComponent>(true, true);

        // Register editor settings
        RegisterEditorSettings();
        
        // Register the dungeon draw editor mode
        FEditorModeRegistry::Get().RegisterMode<FEdModeDungeonLegacy>(
            FEdModeDungeonLegacy::EM_Dungeon, LOCTEXT("DungeonDrawMode", "Draw Dungeon"),
            FSlateIcon(FDungeonArchitectStyle::GetStyleSetName(), "DungeonArchitect.TabIcon", "DungeonArchitect.TabIcon.Small"),
            true, 400
        );
        
        FEditorModeRegistry::Get().RegisterMode<FMGPatternEditMode>(
                    FMGPatternEditMode::EM_PatternEditor, LOCTEXT("PatternEdModeLabel", "Pattern Editor"),
                    FSlateIcon(), false);
        
        // Hook on to the map change event to bind any missing inputs that the samples require
        InputBinderHook = MakeShareable(new FDACustomInputConfigBinderHook);
        InputBinderHook->AddHook();
        
        // Track dungeon actor property change events to handle advanced dungeon details
        DungeonPropertyChangeListener = MakeShareable(new FDungeonPropertyChangeListener);
        DungeonPropertyChangeListener->Initialize();

        UEdGraphSchema_Grammar::GrammarGraphSupport = new FEditorGrammarGraphSupport();
        UEdGraphSchema_GrammarExec::ExecGraphSupport = new FEditorGrammarExecGraphSupport();

        // Create and editor service, so the runtime module can access it
        IDungeonEditorService::Set(MakeShareable(new FDungeonEditorService));

        DungeonItemPlacements.Initialize();
        FLaunchPadSystem::Register();

    }
    
    void OnPostEngineInit() {
        RegisterComponentVisualizer<UPlaceableMarkerComponent, FPlaceableMarkerVisualizer>();
        RegisterComponentVisualizer<UDASceneDebugDataComponent, FDASceneDebugDataComponentVisualizer>();
    }

    virtual void ShutdownModule() override {
        FCoreDelegates::OnPostEngineInit.RemoveAll(this);
        
        FLaunchPadSystem::Unregister();
        DungeonItemPlacements.Release();
        UIExtender.Release();

        // Unregister all the asset types that we registered
        if (FModuleManager::Get().IsModuleLoaded("AssetTools")) {
            IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
            for (int32 Index = 0; Index < CreatedAssetTypeActions.Num(); ++Index) {
                AssetTools.UnregisterAssetTypeActions(CreatedAssetTypeActions[Index].ToSharedRef());
            }
        }
        CreatedAssetTypeActions.Empty();

        // Unregister all the visual node factories
        for (const TSharedPtr<FGraphPanelNodeFactory>& VisualNodeFactory : CreatedVisualNodeFactories) {
            FEdGraphUtilities::UnregisterVisualNodeFactory(VisualNodeFactory);
        }
        CreatedVisualNodeFactories.Empty();

        // Unregister all the visual pin factories
        for (const TSharedPtr<FGraphPanelPinFactory>& VisualPinFactory : CreatedVisualPinFactories) {
            FEdGraphUtilities::UnregisterVisualPinFactory(VisualPinFactory);
        }
        CreatedVisualPinFactories.Empty();

        FEditorModeRegistry::Get().UnregisterMode(FEdModeDungeonLegacy::EM_Dungeon);
        FEditorModeRegistry::Get().UnregisterMode(FMGPatternEditMode::EM_PatternEditor);

        delete UEdGraphSchema_Grammar::GrammarGraphSupport;
        UEdGraphSchema_Grammar::GrammarGraphSupport = nullptr;

        // Unregister the blueprint event autogen
        FKismetEditorUtilities::UnregisterAutoBlueprintNodeCreation(this);

        // Unregister property editor customizations
        if (FModuleManager::Get().IsModuleLoaded("PropertyEditor")) {
            FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
            UnregisterCustomClassLayout(PropertyEditorModule);
            UnregisterCustomPropertyTypeLayout(PropertyEditorModule);
            FFlowEditorTaskCustomizations::UnregisterTaskNodes(PropertyEditorModule);
        }

        // Unregister content browser hooks
        FDungeonArchitectContentBrowserExtensions::RemoveHooks();
        
        if (InputBinderHook.IsValid()) {
            InputBinderHook->RemoveHook();
            InputBinderHook = nullptr;
        }
        
        UnregisterAssetBrokers();
        UnregisterComponentVisualizers();
        UnregisterCommands();
        UnregisterEditorSettings();
        ShutdownStyles();
    }
    
    void RegisterEditorSettings() {
        RegisterSettings<UDungeonCanvasEditorSettings>("Editor", "ContentEditors", "DungeonCanvasEditor",
            LOCTEXT("DungeonCanvasEditorSettingsName", "Dungeon Canvas Editor"),
            LOCTEXT("DungeonCanvasEditorSettingsDescription", "Configure the look and feel of the Dungeon Canvas Editor."));
        
    }
    
    virtual EAssetTypeCategories::Type GetDungeonAssetCategoryBit() const override {
        return DungeonAssetCategoryBit;
    }

public:
    TSharedPtr<FDungeonPropertyChangeListener> DungeonPropertyChangeListener;

private:
    static void RegisterCommands() {
        FDungeonArchitectCommands::Register();
        FThemeGraphAppModeCommands::Register();
        FMarkerGeneratorAppModeCommands::Register();
        FDungeonEditorViewportCommands::Register();
        FDungeonForgeEditorCommands::Register();
        FSnapMapEditorViewportCommands::Register();
        FSnapConnectionEditorCommands::Register();
        FSnapConnectionEditorViewportCommands::Register();
        FFlowEditorCommands::Register();
        FGridFlowEditorViewportCommands::Register();
        FCellFlowDomainEdViewportCommands::Register();
        FDALevelToolbarCommands::Register();
        FDATestRunnerCommands::Register();
        FDungeonCanvasEditorCommands::Register();
        FDungeonToolsEditorCommands::Register();
        FDungeonForgeEditorModeCommands::Register();
        FDungeonThemeEdModeCommands::Register();
    }

    static void UnregisterCommands() {
        FDungeonArchitectCommands::Unregister();
        FThemeGraphAppModeCommands::Unregister();
        FMarkerGeneratorAppModeCommands::Unregister();
        FDungeonEditorViewportCommands::Unregister();
        FDungeonForgeEditorCommands::Unregister();
        FSnapMapEditorViewportCommands::Unregister();
        FSnapConnectionEditorCommands::Unregister();
        FSnapConnectionEditorViewportCommands::Unregister();
        FFlowEditorCommands::Unregister();
        FGridFlowEditorViewportCommands::Unregister();
        FCellFlowDomainEdViewportCommands::Unregister();
        FDALevelToolbarCommands::Unregister();
        FDATestRunnerCommands::Unregister();
        FDungeonCanvasEditorCommands::Unregister();
        FDungeonToolsEditorCommands::Unregister();
        FDungeonForgeEditorModeCommands::Unregister();
        FDungeonThemeEdModeCommands::Unregister();
    }

    static void InitializeStyles() {
        FDungeonArchitectStyle::Initialize();
        FDungeonToolsEditorModeStyles::Initialize();
        FDALaunchPadStyle::Initialize();
    }

    static void ShutdownStyles() {
        FDungeonArchitectStyle::Shutdown();
        FDungeonToolsEditorModeStyles::Shutdown();
        FDALaunchPadStyle::Shutdown();
    }

    void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action) {
        AssetTools.RegisterAssetTypeActions(Action);
        CreatedAssetTypeActions.Add(Action);
    }

    void RegisterVisualNodeFactory(TSharedRef<FGraphPanelNodeFactory> VisualNodeFactory) {
        FEdGraphUtilities::RegisterVisualNodeFactory(VisualNodeFactory);
        CreatedVisualNodeFactories.Add(VisualNodeFactory);
    }

    void RegisterVisualPinFactory(TSharedRef<FGraphPanelPinFactory> VisualPinFactory) {
        FEdGraphUtilities::RegisterVisualPinFactory(VisualPinFactory);
        CreatedVisualPinFactories.Add(VisualPinFactory);
    }

    template<typename TComponent, typename TVisualizer>
    void RegisterComponentVisualizer() {
        RegisterComponentVisualizer(TComponent::StaticClass()->GetFName(),MakeShareable(new TVisualizer));
    }
    
    void RegisterComponentVisualizer(FName ComponentClassName, TSharedPtr<FComponentVisualizer> Visualizer) {
        if (GUnrealEd) {
            GUnrealEd->RegisterComponentVisualizer(ComponentClassName, Visualizer);
        }

        RegisteredComponentClassNames.Add(ComponentClassName);

        if (Visualizer.IsValid()) {
            Visualizer->OnRegister();
        }
    }

    void UnregisterComponentVisualizers() {
        if(GUnrealEd) {
            for(const FName& ClassName : RegisteredComponentClassNames) {
                GUnrealEd->UnregisterComponentVisualizer(ClassName);
            }
            RegisteredComponentClassNames.Reset();
        }
    }

    template<typename TClass, typename TCustomization>
    void RegisterCustomClassLayout(FPropertyEditorModule& PropertyEditorModule) {
        const FName ClassName = TClass::StaticClass()->GetFName();
        PropertyEditorModule.RegisterCustomClassLayout(ClassName, FOnGetDetailCustomizationInstance::CreateStatic(&TCustomization::MakeInstance));
        CustomLayoutClassNames.Add(ClassName);
    }

    void UnregisterCustomClassLayout(FPropertyEditorModule& PropertyEditorModule) {
        for (const FName& ClassName : CustomLayoutClassNames) {
            PropertyEditorModule.UnregisterCustomClassLayout(ClassName);
        }
        CustomLayoutClassNames.Reset();
    }

    template<typename TClass, typename TCustomization>
    void RegisterCustomPropertyTypeLayout(FPropertyEditorModule& PropertyEditorModule) {
        const FName ClassName = TClass::StaticStruct()->GetFName();
        PropertyEditorModule.RegisterCustomPropertyTypeLayout(ClassName, FOnGetPropertyTypeCustomizationInstance::CreateStatic(&TCustomization::MakeInstance));
        CustomPropertyTypeLayoutNames.Add(ClassName);
    }

    void UnregisterCustomPropertyTypeLayout(FPropertyEditorModule& PropertyEditorModule) {
        for (const FName& ClassName : CustomPropertyTypeLayoutNames) {
            PropertyEditorModule.UnregisterCustomPropertyTypeLayout(ClassName);
        }
        CustomPropertyTypeLayoutNames.Reset();
    }
    
    template<typename TBroker, typename TComponent>
    void RegisterAssetBroker(bool bSetAsPrimary, bool bMapComponentForAssets) {
        TSharedPtr<IComponentAssetBroker> AssetBroker = MakeShareable(new TBroker);
        FComponentAssetBrokerage::RegisterBroker(AssetBroker, TComponent::StaticClass(), bSetAsPrimary, bMapComponentForAssets);
        AssetBrokers.Add(AssetBroker);
    }

    void UnregisterAssetBrokers() {
        if (UObjectInitialized()) {
            // Unregister the component brokers
            for (TSharedPtr<IComponentAssetBroker> AssetBroker : AssetBrokers) {
                if (AssetBroker.IsValid()) {
                    FComponentAssetBrokerage::UnregisterBroker(AssetBroker);
                }
            }
        }
    }

    struct FSettingsInfo {
        FName ContainerName;
        FName CategoryName;
        FName SectionName;
    };
    template<typename TClass>
    void RegisterSettings(const FName& ContainerName, const FName& CategoryName, const FName& SectionName, const FText& DisplayName, const FText& Description) {
		if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings")) {
		    SettingsModule->RegisterSettings(ContainerName, CategoryName, SectionName, 
                DisplayName, Description, GetMutableDefault<TClass>()
            );
		    RegisteredSettings.Add({ ContainerName, CategoryName, SectionName });
		}
        
    }

    void UnregisterEditorSettings() {
        if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings")) {
            for (const FSettingsInfo& RegisteredSetting : RegisteredSettings) {
                SettingsModule->UnregisterSettings(RegisteredSetting.ContainerName, RegisteredSetting.CategoryName, RegisteredSetting.SectionName);
            }
        }
        RegisteredSettings.Reset();
    }
    
private:
    /** All created asset type actions.  Cached here so that we can unregister them during shutdown. */
    TArray<TSharedPtr<IAssetTypeActions>> CreatedAssetTypeActions;
    TArray<TSharedPtr<FGraphPanelNodeFactory>> CreatedVisualNodeFactories;
    TArray<TSharedPtr<FGraphPanelPinFactory>> CreatedVisualPinFactories;
    TArray<TSharedPtr<IComponentAssetBroker>> AssetBrokers;
	TArray<FName> RegisteredComponentClassNames;
    TArray<FName> CustomLayoutClassNames;
    TArray<FName> CustomPropertyTypeLayoutNames;

    EAssetTypeCategories::Type DungeonAssetCategoryBit = EAssetTypeCategories::None;

    FDungeonItemsPlacements DungeonItemPlacements;
    FEditorUIExtender UIExtender;
    TSharedPtr<FDACustomInputConfigBinderHook> InputBinderHook;
    TArray<FSettingsInfo> RegisteredSettings;
};

IMPLEMENT_MODULE(FDungeonArchitectEditorModule, DungeonArchitectEditor)


#undef LOCTEXT_NAMESPACE

