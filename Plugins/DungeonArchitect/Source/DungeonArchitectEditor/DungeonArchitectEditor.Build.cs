//$ Copyright 2015-24, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

namespace UnrealBuildTool.Rules
{
    public class DungeonArchitectEditor : ModuleRules
    {
        public DungeonArchitectEditor(ReadOnlyTargetRules Target) : base(Target)
        {
            PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
            bUseUnity = true;
            //PCHUsage = PCHUsageMode.NoPCHs;
            //bUseUnity = false;
            
            IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
            bWarningsAsErrors = true;

            PublicIncludePaths.AddRange(new string[] {
                // ... add public include paths required here ...
            });

            PrivateIncludePaths.AddRange(new string[] {
            });

            PrivateIncludePathModuleNames.AddRange(new[] {
                "Settings",
                "MessageLog"
            });

            PublicDependencyModuleNames.AddRange(new string[]
            {
                "DungeonArchitectRuntime",
                "EditorInteractiveToolsFramework",
                "InteractiveToolsFramework", 
                "StatusBar",
                "Engine",
            });

            PrivateDependencyModuleNames.AddRange(new[] {
                "AddContentDialog",
                "AdvancedPreviewScene",
                "ApplicationCore",
                "AppFramework",
                "AssetRegistry",
                "AssetDefinition",
                "BlueprintGraph",
                "ContentBrowser",
                "Core",
                "CoreUObject",
                "EditorFramework",
                "EditorScriptingUtilities",
                "EditorStyle",
                "EditorWidgets",
                "EditorSubsystem",
                "Foliage",
                "GeometryFramework",
                "GeometryScriptingCore",
                "GeometryScriptingEditor",
                "GraphEditor",
                "InputCore",
                "Json",
                "JsonUtilities",
                "Kismet",
                "KismetCompiler",
                "KismetWidgets",
                "LevelEditor",
                "MainFrame",
                "MaterialEditor",
                "MaterialUtilities",
                "PlacementMode",
                "Projects",
                "PropertyEditor",
                "RenderCore",
                "RHI",
                "Slate",
                "SlateCore",
                "ToolMenus",
                "ToolWidgets",
                "UnrealEd",
                "AssetTools",
                "WorkspaceMenuStructure",
                "ToolPresetEditor",
                "ToolPresetAsset", 
                "ModelingEditorUI",
                "ModelingComponents",
                "ModelingComponentsEditorOnly",
                "WidgetRegistration",
                "EditorConfig",
                "DeveloperSettings",
                "StatusBar",
                "ToolWidgets",
                "HTTP",
                "WebBrowser"
            });

            DynamicallyLoadedModuleNames.AddRange(new string[] {
                // ... add any modules that your module loads dynamically here ...
            });
        }
    }
}
