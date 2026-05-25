//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Utils/DungeonShapes.h"
#include "Frameworks/Snap/Lib/SnapLibrary.h"
#include "Frameworks/Snap/SnapGridFlow/SnapGridFlowModuleDatabase.h"

struct FDungeonPointOfInterest;
class UFlowAbstractGraphBase;

//////////////////////////////////////// Graph Node Impl //////////////////////////////////////////////
class DUNGEONARCHITECTRUNTIME_API FSnapGridFlowGraphNode : public SnapLib::ISnapGraphNode {
public:
    FSnapGridFlowGraphNode(TWeakObjectPtr<UFlowAbstractGraphBase> InGraph, const FGuid& InNodeID);
    virtual FGuid GetNodeID() const override { return NodeID; }
    virtual FName GetCategory() const override;
    virtual TArray<SnapLib::ISnapGraphNodePtr> GetOutgoingNodes(const FGuid& IncomingNodeId) const override;

    FVector GetNodeCoord() const;
    
private:
    FGuid NodeID;
    TWeakObjectPtr<UFlowAbstractGraphBase> Graph;
};

//////////////////////////////////////// Graph Generator //////////////////////////////////////////////
class DUNGEONARCHITECTRUNTIME_API FSnapGridFlowGraphGenerator : public SnapLib::FSnapGraphGenerator {
public:
    FSnapGridFlowGraphGenerator(const SnapLib::IModuleDatabasePtr& InModuleDatabase, const SnapLib::FGrowthStaticState& InStaticState,
            const FVector& InModuleSize, const FVector& InBaseOffset)
        : FSnapGraphGenerator(InModuleDatabase, InStaticState)
        , ModuleSize(InModuleSize)
        , BaseOffset(InBaseOffset)
    {}

protected:
    virtual bool ModuleOccludes(const SnapLib::FModuleNodePtr& ModuleNode, const SnapLib::ISnapGraphNodePtr& MissionNode, const TArray<SnapLib::FOcclusionEntry>& OcclusionList) override;
    virtual TArray<FTransform> GetStartingNodeTransforms(const SnapLib::FModuleNodePtr& ModuleNode, const SnapLib::ISnapGraphNodePtr& MissionNode) override;

private:
    FVector ModuleSize;
    FVector BaseOffset;
};

//////////////////////////////////////// Module Database Adapter //////////////////////////////////////////////
class DUNGEONARCHITECTRUNTIME_API FSnapGridFlowGraphModDBItemImpl
    : public SnapLib::IModuleDatabaseItem
{
public:
    explicit FSnapGridFlowGraphModDBItemImpl(const FSnapGridFlowModuleDatabaseItem& InItem) : Item(InItem) {}
    FSnapGridFlowModuleDatabaseItem GetItem() const { return Item; }

    virtual FBox GetBounds() const override { return Item.ModuleBounds; }
    virtual TSoftObjectPtr<UWorld> GetLevel() const override { return Item.Level; }
    virtual const TMap<FName, TSoftObjectPtr<UWorld>>& GetThemedLevels() const override { return Item.ThemedLevels; }
    virtual FName GetCategory() const override { return Item.Category; }
    virtual TArray<FName> GetTags() const override { return Item.Tags; }
    virtual bool ShouldAllowRotation() const override { return Item.bAllowRotation; }
    virtual SnapLib::FModuleNodePtr CreateModuleNode(const FGuid& InNodeId) override;
    virtual FDungeonCanvasRoomShapeTextureList GetCanvasRoomShapeTextures() override;
    virtual TArray<FDungeonPointOfInterest> GetPointsOfInterest() override;
    virtual FDAShapeList GetBoundShapes() const override { return Item.ModuleBoundShapes; }

private:
    FSnapGridFlowModuleDatabaseItem Item;
};

class DUNGEONARCHITECTRUNTIME_API FSnapGridFlowModuleDatabaseImpl final : public SnapLib::IModuleDatabase {
public:
    explicit FSnapGridFlowModuleDatabaseImpl(USnapGridFlowModuleDatabase* ModuleDB);

    FORCEINLINE FVector GetChunkSize() const { return ChunkSize; }
    
private:
    FVector ChunkSize;
};
typedef TSharedPtr<FSnapGridFlowModuleDatabaseImpl> FSnapGridFlowModuleDatabaseImplPtr;

