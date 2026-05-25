//$ Copyright 2015-24, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

namespace UnrealBuildTool.Rules
{
	public class DungeonArchitectRuntime : ModuleRules
	{
		public DungeonArchitectRuntime(ReadOnlyTargetRules Target) : base(Target)
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

			PublicDependencyModuleNames.AddRange(new string[] {
				"Core",
				"Engine",
				"NetCore",
				"InputCore",
				"RenderCore", 
				"PhysicsCore",
				"GeometryCore",
				"GeometryFramework",
				"GeometryScriptingCore",
				"UMG",
				"ModularGameplay",
				"CommonInput",
				"CommonUI",
				"GameplayTags",
				"AIModule",
				"RHI",
				"Renderer",
				"PCG",
				"Slate",
				"SlateCore",
				"PCGCompute",
				"MeshDescription",
				"StaticMeshDescription",
				"DeveloperSettings",
			});

			PrivateDependencyModuleNames.AddRange(new string[] {
				"CoreUObject",
				"Foliage",
				"Landscape",
				"AssetRegistry",
				"NavigationSystem",
				"Projects", 
			});

			DynamicallyLoadedModuleNames.AddRange(new string[] {
				// ... add any modules that your module loads dynamically here ...
			});
		}
	}
}
