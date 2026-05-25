//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "CoreMinimal.h"

#include "Frameworks/FlowImpl/SnapGridFlow/LayoutGraph/SnapGridFlowAbstractGraph.h"
#include "Frameworks/Snap/SnapGridFlow/SnapGridFlowLibrary.h"
#include "Frameworks/Snap/SnapGridFlow/SnapGridFlowModuleResolver.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSnapGridFlowResolveTest, "DungeonArchitectTests.SnapGridFlow.Resolve", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)



bool FSnapGridFlowResolveTest::RunTest(const FString& Parameters) {
	USnapGridFlowAbstractGraph* AbstractGraph = nullptr; // TODO:
	USnapGridFlowModuleDatabase* ModuleDatabase = NewObject<USnapGridFlowModuleDatabase>(); // TODO:
	
	
	FSnapGridFlowModuleResolverSettings ResolveSettings;
	ResolveSettings.Seed = 0;
	ResolveSettings.ChunkSize = FVector(1000, 1000, 500);
    
	const FSnapGridFlowModuleDatabaseImplPtr ModDB = MakeShareable(new FSnapGridFlowModuleDatabaseImpl(ModuleDatabase));
	const FSnapGridFlowModuleResolver ModuleResolver(ModDB, ResolveSettings);

	TArray<SnapLib::FModuleNodePtr> ResolvedNodes;
	return ModuleResolver.Resolve(AbstractGraph, ResolvedNodes);
	
	return true;
}

