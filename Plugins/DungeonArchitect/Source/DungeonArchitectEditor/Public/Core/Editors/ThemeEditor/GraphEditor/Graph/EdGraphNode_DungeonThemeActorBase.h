//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeBase.h"
#include "EdGraphNode_DungeonThemeActorBase.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(DungeonNodeLog, Log, All);

class UDungeonSelectorLogic;
class UDungeonTransformLogic;
class UDungeonSpawnLogic;
class UProceduralDungeonTransformLogic;
class UGenericDungeonSelectorLogic;
class FDAAssetThumbnailCacheManager;

UCLASS()
class DUNGEONARCHITECTEDITOR_API UEdGraphNode_DungeonThemeActorBase : public UEdGraphNode_DungeonThemeBase {
    GENERATED_UCLASS_BODY()

    // Begin UEdGraphNode interface.
    virtual void AllocateDefaultPins() override;
    virtual FLinearColor GetNodeTitleColor() const override;
    virtual FText GetTooltipText() const override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    // End UEdGraphNode interface.

    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;

public:
    virtual UEdGraphPin* GetInputPin() const override { return Pins[0]; }
    virtual UEdGraphPin* GetOutputPin() const override { return Pins[1]; }
    virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
    virtual void NodeConnectionListChanged() override;
    virtual UObject* GetNodeAssetObject(UObject* Outer) const { return nullptr; }
    virtual TArray<UObject*> GetThumbnailAssetObjects() const;
	virtual FLinearColor GetBorderColor();

public:
    /**
      The probability that this node would be processed.   
      A value of 1 would always process this node and spawn a mesh.  
      A probability of 0.5 would spawn this mesh 50% of the time
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    float Probability;

    /** Deprecated. Use Probability instead */
    UPROPERTY()
    float Affinity;

    /**
      You can have multiple mesh nodes attached to the same marker type.
      Each node is processed in order from left to right until all of them
      are processed or a node has this flag checked. 
      This lets you attach multiple meshes / actors in the same marker point
      (with possible varying probabilities)
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon, meta = (DisplayName = "Stop If Selected"))
    bool ConsumeOnAttach = true;

    /** Use this to adjust the mesh pivot, rotation and scale of your mesh, when it is attached to the marker */
    UPROPERTY(BlueprintReadWrite, Category = Dungeon)
    FTransform Offset;

	/** Extra tags to add to the actor after it spawns */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Actor)
	TArray<FName> ActorTags;
	
    /** Use blueprint based selection logic */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection Logic")
    bool bUseSelectionLogic = false;

    /** 
      Determines if we need to multiply the Affinity with the selection logic's result
      If true, the selection logic fully controls the outcome of the selection process, i.e either select(1) or deselect(0)  
      If false, then the Affinity would be multiplied with the selector's result
      Uncheck this if you also want the Affinity to be used along with your selection logic
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection Logic", meta=(EditCondition="bUseSelectionLogic"))
    bool bLogicOverridesAffinity = true;

    /**
    Create a blueprint based selection logic by clicking the plus sign.
    Override the SelectNode function in the blueprint to decide if
    this node is inserted into the scene based on your blueprint logic
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, SimpleDisplay, Category = "Selection Logic", meta=(EditCondition="bUseSelectionLogic"))
    TArray<TObjectPtr<UDungeonSelectorLogic>> SelectionLogics;

	/* Run common non-builder based Selection logics. These logic blueprints are re-usable across different builders */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, SimpleDisplay, Category = "Selection Logic", meta=(EditCondition="bUseSelectionLogic"))
	TArray<TObjectPtr<UGenericDungeonSelectorLogic>> GenericSelectionLogics;
	
    /** Use blueprint based transform logic */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform Logic")
    bool bUseTransformLogic = false;

    /** Create a blueprint to decide on the transform offset that needs to be applied on this node */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, SimpleDisplay, Category = "Transform Logic", meta=(EditCondition="bUseTransformLogic"))
    TArray<TObjectPtr<UDungeonTransformLogic>> TransformLogics;

	/** Use commonly used procedural transform logic */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform Logic")
	bool bUseProceduralTransformLogic = false;

	/* Run commonly used procedural transform logics */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, SimpleDisplay, Category = "Transform Logic", meta=(EditCondition="bUseProceduralTransformLogic"))
	TArray<TObjectPtr<UProceduralDungeonTransformLogic>> ProceduralTransformLogics;
	
    /** Use blueprint based selection logic */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Logic")
    bool bUseSpawnLogic = false;

    /** Create a blueprint to decide on the transform offset that needs to be applied on this node */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, SimpleDisplay, Category = "Spawn Logic", meta=(EditCondition="bUseSpawnLogic"))
    TArray<TObjectPtr<UDungeonSpawnLogic>> SpawnLogics;

	/** Enable this if you want the mesh to sit correctly on the noise displaced voxel mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Voxel)
	bool bAlignToVoxelGeometry = false;
	
    UPROPERTY()
    int32 ExecutionOrder = 0;

	TSharedPtr<FDAAssetThumbnailCacheManager> GetThumbnailCacheManager() const { return ThumbnailCacheManager; }
	
private:
	TSharedPtr<FDAAssetThumbnailCacheManager> ThumbnailCacheManager;
};

