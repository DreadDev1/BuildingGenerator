//$ Copyright 2015-24, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

namespace UnrealBuildTool.Rules
{
	public class DungeonArchitectGameplay : ModuleRules
	{
		public DungeonArchitectGameplay(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
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
				"PhysicsCore",
				"Slate",
				"SlateCore",
				"UMG",
				"ModularGameplay",
				"CommonInput",
				"CommonUI",
				"GameplayAbilities",
				"GameplayTasks",
				"GameplayTags",
				"ModularGameplay",
				"EnhancedInput",
				"Niagara"
			});

			PrivateDependencyModuleNames.AddRange(new string[] {
				"CoreUObject",
				"NavigationSystem",
				"AIModule",
			});

			DynamicallyLoadedModuleNames.AddRange(new string[] {
				// ... add any modules that your module loads dynamically here ...
			});
			
			
			SetupIrisSupport(Target);
		}
	}
}
