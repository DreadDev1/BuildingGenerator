//$ Copyright 2015-24, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

namespace UnrealBuildTool.Rules
{
	public class DungeonArchitectGameplaySupport : ModuleRules
	{
		public DungeonArchitectGameplaySupport(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
			IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
			bWarningsAsErrors = true;
			
			PublicIncludePaths.AddRange(new string[] {
				// ... add public include paths required here ...
			});

			PublicDependencyModuleNames.AddRange(new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"KismetCompiler",
				"PropertyEditor",
				"DungeonArchitectGameplay",
				"UnrealEd"
			});

			PrivateDependencyModuleNames.AddRange(new string[] {
				"BlueprintGraph",
			});

			DynamicallyLoadedModuleNames.AddRange(new string[] {
				// ... add any modules that your module loads dynamically here ...
			});
		}
	}
}
