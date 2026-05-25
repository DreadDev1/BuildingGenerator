//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "SGraphNode.h"

class UFlowExecEdGraphNodeBase;
class SHorizontalBox;

class DUNGEONARCHITECTEDITOR_API SGraphNode_FlowExecNode : public SGraphNode {
public:
    SLATE_BEGIN_ARGS(SGraphNode_FlowExecNode) {}
    SLATE_END_ARGS()

    /** Constructs this widget with InArgs */
    void Construct(const FArguments& InArgs, UFlowExecEdGraphNodeBase* InNode);

    // SGraphNode interface
    virtual void UpdateGraphNode() override;
    virtual void CreatePinWidgets() override;
    virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
    // End of SGraphNode interface

    void SetBorderColor(const FLinearColor& InNodeBorderColor);

    const FSlateBrush* GetBorderImage() const;

protected:
    FSlateColor GetBorderBackgroundColor() const;
    virtual const FSlateBrush* GetNameIcon() const;
    FText GetNodeDescriptionText() const;
    EVisibility GetNodeDescriptionVisibility() const;
    FText GetNodeErrorText() const;

protected:
    FLinearColor DefaultNodeBorderColor;
    TSharedPtr<SErrorText> ErrorText;
    TSharedPtr<IErrorReportingWidget> ErrorReporting;
};

