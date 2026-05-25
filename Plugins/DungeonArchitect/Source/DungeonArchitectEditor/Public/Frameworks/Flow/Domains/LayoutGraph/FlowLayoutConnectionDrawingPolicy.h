//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "ConnectionDrawingPolicy.h"

class SFlowItemOverlay;
class SGraphNode_FlowLayoutNode;

// This class draws the connections for an UEdGraph with a behavior tree schema
class DUNGEONARCHITECTEDITOR_API FFlowLayoutConnectionDrawingPolicy : public FConnectionDrawingPolicy {
public:
    //
    FFlowLayoutConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor,
                                             const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements,
                                             UEdGraph* InGraphObj);

    // FConnectionDrawingPolicy interface 
    virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/
                                      FConnectionParams& Params) override;
    virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& PinGeometries, FArrangedChildren& ArrangedNodes) override;
    virtual void DrawSplineWithArrow(const FGeometry& StartGeom, const FGeometry& EndGeom,
                                     const FConnectionParams& Params) override;
    virtual void DrawSplineWithArrow(const FVector2D& StartPoint, const FVector2D& EndPoint,
                                     const FConnectionParams& Params) override;
    virtual void DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint,
                                      UEdGraphPin* Pin) override;
    virtual void DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End,
                                const FConnectionParams& Params) override;

    virtual void DetermineLinkGeometry(
        FArrangedChildren& ArrangedNodes,
        TSharedRef<SWidget>& OutputPinWidget,
        UEdGraphPin* OutputPin,
        UEdGraphPin* InputPin,
        /*out*/ FArrangedWidget*& StartWidgetGeometry,
        /*out*/ FArrangedWidget*& EndWidgetGeometry
    ) override;

    // End of FConnectionDrawingPolicy interface

    struct FItemWidgetInfo {
        TWeakPtr<SFlowItemOverlay> ItemWidget;
        FVector2D ItemCenter;
        float ItemRadius = 0;
    };

protected:
    void Internal_DrawLineWithArrow(const FVector2D& StartAnchorPoint, const FVector2D& EndAnchorPoint,
                                    const FConnectionParams& Params, bool bForceForeground = false);

protected:
    UEdGraph* GraphObj;
    TMap<FGuid, int32> NodeWidgetMap;
    TMap<FGuid, FItemWidgetInfo> ItemWidgetMap;
};

