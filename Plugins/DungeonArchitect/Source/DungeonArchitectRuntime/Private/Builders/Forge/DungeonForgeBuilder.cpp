//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/Forge/DungeonForgeBuilder.h"

#include "Builders/Forge/DungeonForgeConfig.h"
#include "Builders/Forge/DungeonForgeModel.h"
#include "Builders/Forge/DungeonForgeQuery.h"
#include "Builders/Forge/DungeonForgeToolData.h"
#include "Core/Dungeon.h"
#include "Frameworks/Forge/DungeonForgeResourceManager.h"

void UDungeonForgeBuilder::BuildDungeonImpl(UWorld* World) {
	if (ResourceManager) {
		ResourceManager->ReleaseResources();
		ResourceManager = nullptr;
	}
	ResourceManager = NewObject<UDungeonForgeResourceManager>(this);
	
	if (ADungeon* OuterDungeon = Cast<ADungeon>(GetOuter())) {
		if (UDungeonForgeConfig* ForgeConfig = Cast<UDungeonForgeConfig>(OuterDungeon->GetConfig())) {
			if (const UDungeonForgeAsset* ForgeAsset = ForgeConfig->DungeonForgeAsset.LoadSynchronous()) {
				Dungeon = Cast<ADungeon>(GetOuter());
				GraphExecutor.Execute(ForgeAsset->ForgeGraph, ResourceManager, Dungeon, &GraphOutputCollector);
			}
		}
	}
}

void UDungeonForgeBuilder::DestroyDungeonImpl(UWorld* World) {
	if (GraphExecutor.IsRunning()) {
		GraphExecutor.Abort();
	}

	if (ResourceManager) {
		ResourceManager->ReleaseResources();
		ResourceManager = nullptr;
	}
	
	Super::DestroyDungeonImpl(World);
}

bool UDungeonForgeBuilder::CanBuildDungeon(FString& OutMessage) {
	if (ADungeon* OuterDungeon = Cast<ADungeon>(GetOuter())) {
		UDungeonForgeConfig* ForgeConfig = Cast<UDungeonForgeConfig>(OuterDungeon->GetConfig());

		if (!ForgeConfig) {
			OutMessage = "Dungeon is not initialized correctly";
			return false;
		}

		const UDungeonForgeAsset* ForgeAsset = ForgeConfig->DungeonForgeAsset.LoadSynchronous();
		if (!ForgeAsset) {
			OutMessage = "Dungeon Forge asset has not been assigned";
			return false;
		}
	}
	else {
		OutMessage = "Dungeon is not initialized correctly";
		return false;
	}

	return true;
}

FDungeonBuilderCapabilities UDungeonForgeBuilder::GetCapabilities() {
	FDungeonBuilderCapabilities Capabilities;
	Capabilities.bSupportsTheming = false;
	Capabilities.bSupportsLevelStreaming = false;
	Capabilities.bSupportsOverlappingFloors = false;
	return Capabilities;
}

TSubclassOf<UDungeonModel> UDungeonForgeBuilder::GetModelClass() {
	return UDungeonForgeModel::StaticClass();
}

TSubclassOf<UDungeonConfig> UDungeonForgeBuilder::GetConfigClass() {
	return UDungeonForgeConfig::StaticClass();
}

TSubclassOf<UDungeonQuery> UDungeonForgeBuilder::GetQueryClass() {
	return UDungeonForgeQuery::StaticClass();
}

TSubclassOf<UDungeonToolData> UDungeonForgeBuilder::GetToolDataClass() {
	return UDungeonForgeToolData::StaticClass();
}

bool UDungeonForgeBuilder::IsGenerating() const {
	return GraphExecutor.IsRunning();
}

void UDungeonForgeBuilder::AbortGeneration() {
	GraphExecutor.Abort();
}

TStatId UDungeonForgeBuilder::GetStatId() const {
	return TStatId();
}

void UDungeonForgeBuilder::Tick(float DeltaTime) {
	GraphExecutor.Tick(DeltaTime);
}

bool UDungeonForgeBuilder::IsTickable() const {
	return GraphExecutor.IsRunning();
}

UWorld* UDungeonForgeBuilder::GetTickableGameObjectWorld() const {
	return GetWorld();
}

const FDungeonForgeContext* UDungeonForgeBuilder::GetNodeInstanceContext(const FGuid& InNodeGuid) const {
	return GraphOutputCollector.GetNodeOutput(InNodeGuid);
}

FDungeonForgeContext* UDungeonForgeBuilder::GetNodeInstanceContext(const FGuid& InNodeGuid) {
	return GraphOutputCollector.GetNodeOutput(InNodeGuid);
}


