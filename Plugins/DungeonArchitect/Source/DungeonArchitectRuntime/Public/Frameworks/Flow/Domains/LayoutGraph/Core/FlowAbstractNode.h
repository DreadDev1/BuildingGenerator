//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractItem.h"
#include "FlowAbstractNode.generated.h"

UCLASS(BlueprintType)
class DUNGEONARCHITECTRUNTIME_API UFlowAbstractNode : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FGuid NodeId;
    
    UPROPERTY(BlueprintReadOnly, Category=Dungeon)
    bool bActive = false;

    UPROPERTY()
    FLinearColor Color = FLinearColor::Green;
    
    UPROPERTY(BlueprintReadOnly, Category=Dungeon)
    FVector Coord = FVector::ZeroVector;
    
    UPROPERTY(BlueprintReadOnly, Category=Dungeon)
    FString PathName;
    
    UPROPERTY(BlueprintReadOnly, Category=Dungeon)
    TArray<TObjectPtr<UFlowGraphItem>> NodeItems;
    
    /** This node may be a composite node which was created by merging these nodes */
    UPROPERTY()
    TArray<TObjectPtr<UFlowAbstractNode>> MergedCompositeNodes;

    // Node data saved for use with another domain further down the lane (e.g. tilemap data, voxel data etc)
    UPROPERTY()
    TArray<TObjectPtr<UObject>> CrossDomainNodeData;

    /** How far is this node from the start of this branch */
    UPROPERTY(BlueprintReadOnly, Category=Dungeon)
    int32 PathIndex = INDEX_NONE;
    
    /** The length of this branch */
    UPROPERTY(BlueprintReadOnly, Category=Dungeon)
    int32 PathLength = 0;

    UPROPERTY(BlueprintReadOnly, Category=Dungeon)
    TSet<FName> Tags;
    
public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category=Dungeon)
    bool ContainsItem(EFlowGraphItemType ItemType, int& Count);
    
public:
    /*
    template<typename TItem>
    UFlowGraphItem* CreateNewItem() {
        TItem* NewItem = NewObject<TItem>(this, TItem::StaticClass());
        NodeItems.Add(NewItem);
        return NewItem;
    }
    */

    UFlowGraphItem* CreateNewItem(const TSubclassOf<UFlowGraphItem> InItemClass) {
        UFlowGraphItem* NewItem = NewObject<UFlowGraphItem>(this, InItemClass);
        NodeItems.Add(NewItem);
        return NewItem;
    }

    TSet<FName> GetHierarchyTags() const;
    
    /** Gets the domain data attached to this node. Adds a new one if not available */
    template<typename T>
    T* FindOrAddDomainData() {
        for (UObject* Data : CrossDomainNodeData) {
            if (Data->GetClass() == T::StaticClass()) {
                return Cast<T>(Data);
            }
        }
        T* Data = NewObject<T>(this);
        CrossDomainNodeData.Add(Data);
        return Data;
    }

    /** Gets the domain data attached to this node. Returns null if not found */
    template<typename T>
    T* FindDomainData() const {
        for (UObject* Data : CrossDomainNodeData) {
            if (Data->GetClass() == T::StaticClass()) {
                return Cast<T>(Data);
            }
        }
        return nullptr;
    }
    
    virtual UFlowAbstractNode* Clone(UObject* Outer);
};


UCLASS(BlueprintType)
class DUNGEONARCHITECTRUNTIME_API UFlowAbstractDoorNode : public UFlowAbstractNode {
    GENERATED_BODY()
public:
};


USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FFlowAbstractNodeGroup {
    GENERATED_BODY()

    UPROPERTY()
    FGuid GroupId;
    
    UPROPERTY()
    TArray<FGuid> GroupNodes;
};

