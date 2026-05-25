//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/GraphGrammar/ExecutionGraph/Widgets/SGraphPin_GrammarExecNodePin.h"


void SGraphPin_GrammarExecNodePin::Construct(const FArguments& InArgs, UEdGraphPin* InPin) {
    this->SetCursor(EMouseCursor::Default);

    bShowLabel = true;
    IsEditable = true;

    GraphPinObj = InPin;
    check(GraphPinObj != NULL);

    const UEdGraphSchema* Schema = GraphPinObj->GetSchema();
    check(Schema);

    // Set up a hover for pins that is tinted the color of the pin.
    SBorder::Construct(SBorder::FArguments()
                       .BorderImage(this, &SGraphPin_GrammarExecNodePin::GetPinBorder)
                       .BorderBackgroundColor(this, &SGraphPin_GrammarExecNodePin::GetPinColor)
                       .OnMouseButtonDown(this, &SGraphPin_GrammarExecNodePin::OnPinMouseDown)
                       .Cursor(this, &SGraphPin_GrammarExecNodePin::GetPinCursor)
                       .Padding(FMargin(5.0f))
    );
}

TSharedRef<SWidget> SGraphPin_GrammarExecNodePin::GetDefaultValueWidget() {
    return SNew(STextBlock);
}

const FSlateBrush* SGraphPin_GrammarExecNodePin::GetPinBorder() const {
    return FAppStyle::GetBrush(TEXT("Graph.StateNode.Body"));
}

FSlateColor SGraphPin_GrammarExecNodePin::GetPinColor() const {
    static const FLinearColor MeshPinColor(1.0f, 0.7f, 0.0f);
    static const FLinearColor MarkerPinColor(0.3f, 0.3f, 1.0f);
    static const FLinearColor DarkColor(0.02f, 0.02f, 0.02f);
    if (!IsHovered()) {
        return DarkColor;
    }
    return MeshPinColor;
}

