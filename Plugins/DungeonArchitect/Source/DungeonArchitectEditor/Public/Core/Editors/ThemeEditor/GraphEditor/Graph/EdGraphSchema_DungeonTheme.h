//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraphSchema_DungeonTheme.generated.h"

class UEdGraphNode_DungeonThemeBase;
class UEdGraphNode_DungeonThemeMarker;

UCLASS()
class UEdGraphSchema_DungeonTheme : public UEdGraphSchema {
    GENERATED_UCLASS_BODY()

    // Begin EdGraphSchema interface
    virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
    virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
    virtual class FConnectionDrawingPolicy* CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID,
                                                                          float InZoomFactor, const FSlateRect& InClippingRect,
                                                                          class FSlateWindowElementList& InDrawElements,
                                                                          class UEdGraph* InGraphObj) const override;
    virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;
    virtual bool ShouldHidePinDefaultValue(UEdGraphPin* Pin) const override;
    virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;
	virtual TSharedPtr<FEdGraphSchemaAction> GetCreateCommentAction() const override;
	virtual bool ShouldAlwaysPurgeOnModification() const override { return false; }

	virtual bool IsCacheVisualizationOutOfDate(int32 InVisualizationCacheID) const override;
	virtual int32 GetCurrentVisualizationCacheID() const override;
	virtual void ForceVisualizationCacheClear() const override;
	
    // End EdGraphSchema interface

    void GetActionList(TArray<TSharedPtr<FEdGraphSchemaAction>>& OutActions, const UEdGraph* Graph,
                       UEdGraph* OwnerOfTemporaries, bool bShowNewMesh = true, bool bShowNewMarker = true,
                       bool bShowMarkerEmitters = true) const;

private:
	static int32 CurrentCacheRefreshID;
};


/** Action to add a comment node to the graph */
USTRUCT()
struct FDungeonThemeSchemaAction_NewComment : public FEdGraphSchemaAction
{
	GENERATED_BODY();

	// Simple type info
	static FName StaticGetTypeId() {static FName Type("FDungeonSchemaAction_NewComment"); return Type;}
	virtual FName GetTypeId() const override { return StaticGetTypeId(); } 

	FDungeonThemeSchemaAction_NewComment() 
        : FEdGraphSchemaAction()
	{}

	FDungeonThemeSchemaAction_NewComment(FText InNodeCategory, FText InMenuDesc, FText InToolTip, const int32 InGrouping)
        : FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping)
	{}

	//~ Begin FEdGraphSchemaAction Interface
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	//~ End FEdGraphSchemaAction Interface
};

class FDungeonThemeGraphCycleDetector {
public:
	static bool HasCycles(const UEdGraphPin* A, const UEdGraphPin* B, TArray<UEdGraphNode_DungeonThemeBase*>& OutCyclePath);

private:
	static bool FindCycle(const UEdGraphPin* A, const UEdGraphPin* B, UEdGraphNode_DungeonThemeBase* InNode, TArray<UEdGraphNode_DungeonThemeBase*>& OutPath);
};
